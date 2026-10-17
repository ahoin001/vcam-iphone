import Accelerate
import CoreVideo
import Foundation
import Vision

final class PortraitProcessor: @unchecked Sendable {
    private let lock = NSLock()
    private var command = PortraitCommand.off
    private let request = VNGeneratePersonSegmentationRequest()
    private let scratch = Scratch()
    private var pool: CVPixelBufferPool?
    private var poolWidth = 0
    private var poolHeight = 0
    private var maskWidth = 0
    private var maskHeight = 0
    private var hasPreviousMask = false
    private var frameIndex = 0
    private var busy = false

    init() {
        request.outputPixelFormat = kCVPixelFormatType_OneComponent8
        request.qualityLevel = .fast
    }

    func set(_ command: PortraitCommand) {
        lock.lock()
        self.command = command
        if command.grade == .off {
            hasPreviousMask = false
            frameIndex = 0
        }
        lock.unlock()
    }

    func process(_ source: CVPixelBuffer) -> CVPixelBuffer {
        lock.lock()
        let command = command
        let wasBusy = busy
        if command.grade == .balanced || command.grade == .fast, command.strength > 0.01, !wasBusy {
            busy = true
        }
        lock.unlock()

        guard command.grade == .balanced || command.grade == .fast, command.strength > 0.01 else {
            return source
        }
        if wasBusy {
            return source
        }
        defer {
            lock.lock()
            busy = false
            lock.unlock()
        }

        guard CVPixelBufferGetPixelFormatType(source) == kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange else {
            return source
        }
        let width = CVPixelBufferGetWidth(source)
        let height = CVPixelBufferGetHeight(source)
        guard width >= 16, height >= 16, width.isMultiple(of: 2), height.isMultiple(of: 2) else {
            return source
        }
        prepare(width: width, height: height)

        frameIndex &+= 1
        let refreshMask = !hasPreviousMask || shouldRefreshMask(grade: command.grade, width: width, height: height)
        if refreshMask {
            guard let mask = personMask(from: source, grade: command.grade), maskMeansPerson(mask) else {
                return source
            }
            // Mask planes live in scratch; personMask already wrote feathered.
            _ = mask.width
        } else if !hasPreviousMask {
            return source
        }

        guard let feathered = scratch.plane("maskFeathered", width: maskWidth, height: maskHeight),
              hasPreviousMask else {
            return source
        }
        guard let output = outputBuffer(width: width, height: height) else { return source }
        CVPixelBufferLockBaseAddress(source, .readOnly)
        CVPixelBufferLockBaseAddress(output, [])
        defer {
            CVPixelBufferUnlockBaseAddress(output, [])
            CVPixelBufferUnlockBaseAddress(source, .readOnly)
        }
        guard composite(source: source, output: output, mask: feathered, command: command) else {
            return source
        }
        if let attachments = CVBufferCopyAttachments(source, .shouldPropagate) {
            CVBufferSetAttachments(output, attachments, .shouldPropagate)
        }
        return output
    }

    private func shouldRefreshMask(grade: PortraitGrade, width: Int, height: Int) -> Bool {
        let heavy = height > 1080 || width > 1920
        let interval = grade == .fast || heavy ? 3 : 2
        return frameIndex % interval == 0
    }

    private func personMask(from source: CVPixelBuffer, grade: PortraitGrade) -> vImage_Buffer? {
        request.qualityLevel = .fast
        let handler = VNImageRequestHandler(cvPixelBuffer: source, orientation: .up, options: [:])
        guard (try? handler.perform([request])) != nil,
              let observation = request.results?.first else { return nil }
        let mask = observation.pixelBuffer
        CVPixelBufferLockBaseAddress(mask, .readOnly)
        defer { CVPixelBufferUnlockBaseAddress(mask, .readOnly) }
        let width = CVPixelBufferGetWidth(mask)
        let height = CVPixelBufferGetHeight(mask)
        guard width > 1, height > 1, let base = CVPixelBufferGetBaseAddress(mask) else { return nil }
        if width != maskWidth || height != maskHeight {
            hasPreviousMask = false
            maskWidth = width
            maskHeight = height
        }
        guard var raw = scratch.plane("maskRaw", width: width, height: height),
              var previous = scratch.plane("maskPrev", width: width, height: height),
              var dilated = scratch.plane("maskDilated", width: width, height: height),
              var feathered = scratch.plane("maskFeathered", width: width, height: height) else { return nil }
        var incoming = vImage_Buffer(
            data: base,
            height: vImagePixelCount(height),
            width: vImagePixelCount(width),
            rowBytes: CVPixelBufferGetBytesPerRow(mask)
        )
        guard vImageScale_Planar8(&incoming, &raw, nil, vImage_Flags(kvImageDoNotTile)) == kvImageNoError else {
            return nil
        }
        if hasPreviousMask {
            blend(current: &raw, previous: previous, newWeight: 140)
        }
        guard copy(raw, into: &previous) else { return nil }
        hasPreviousMask = true
        guard vImageMax_Planar8(&raw, &dilated, nil, 0, 0, 3, 3, vImage_Flags(kvImageEdgeExtend | kvImageDoNotTile)) == kvImageNoError else {
            return nil
        }
        let feather: UInt32 = grade == .fast ? 3 : 5
        guard vImageBoxConvolve_Planar8(&dilated, &feathered, nil, 0, 0, feather, feather, 0, vImage_Flags(kvImageEdgeExtend | kvImageDoNotTile)) == kvImageNoError else {
            return nil
        }
        return feathered
    }

    private func maskMeansPerson(_ mask: vImage_Buffer) -> Bool {
        let width = Int(mask.width)
        let height = Int(mask.height)
        guard let data = mask.data else { return false }
        let step = max(width / 32, 1)
        for y in stride(from: 0, to: height, by: step) {
            let row = data.advanced(by: y * mask.rowBytes).assumingMemoryBound(to: UInt8.self)
            for x in stride(from: 0, to: width, by: step) where row[x] >= 8 {
                return true
            }
        }
        return false
    }

    private func composite(source: CVPixelBuffer, output: CVPixelBuffer, mask: vImage_Buffer, command: PortraitCommand) -> Bool {
        guard var sourceY = plane(source, index: 0),
              let sourceC = plane(source, index: 1),
              var destY = plane(output, index: 0),
              var destC = plane(output, index: 1) else { return false }
        var mask = mask
        let yWidth = Int(sourceY.width)
        let yHeight = Int(sourceY.height)
        let cWidth = Int(sourceC.width)
        let cHeight = Int(sourceC.height)
        guard var maskY = scratch.plane("maskY", width: yWidth, height: yHeight),
              var maskC = scratch.plane("maskC", width: cWidth, height: cHeight),
              var opaqueY = scratch.plane("opaqueY", width: yWidth, height: yHeight, fill: 255),
              var opaqueC = scratch.plane("opaqueC", width: cWidth, height: cHeight, fill: 255) else { return false }
        let maskFlags = vImage_Flags(kvImageDoNotTile)
        guard vImageScale_Planar8(&mask, &maskY, nil, maskFlags) == kvImageNoError,
              vImageScale_Planar8(&mask, &maskC, nil, maskFlags) == kvImageNoError else { return false }

        let divisor = command.grade == .fast ? 8 : 6
        guard blur(sourceY, into: &destY, name: "y", divisor: divisor, strength: command.strength, grade: command.grade),
              var blurredY = scratch.plane("yFull", width: yWidth, height: yHeight) else { return false }
        guard copy(destY, into: &blurredY) else { return false }
        guard vImageAlphaBlend_Planar8(&sourceY, &maskY, &blurredY, &opaqueY, &opaqueY, &destY, vImage_Flags(kvImageDoNotTile)) == kvImageNoError else {
            return false
        }

        guard var cb = scratch.plane("cb", width: cWidth, height: cHeight),
              var cr = scratch.plane("cr", width: cWidth, height: cHeight),
              var blurredCB = scratch.plane("cbBlurred", width: cWidth, height: cHeight),
              var blurredCR = scratch.plane("crBlurred", width: cWidth, height: cHeight),
              var outCB = scratch.plane("cbOut", width: cWidth, height: cHeight),
              var outCR = scratch.plane("crOut", width: cWidth, height: cHeight) else { return false }
        guard splitChroma(sourceC, cb: &cb, cr: &cr) else { return false }
        let chromaDivisor = max(2, divisor / 2)
        guard blur(cb, into: &blurredCB, name: "cb", divisor: chromaDivisor, strength: command.strength, grade: command.grade),
              blur(cr, into: &blurredCR, name: "cr", divisor: chromaDivisor, strength: command.strength, grade: command.grade) else { return false }
        guard vImageAlphaBlend_Planar8(&cb, &maskC, &blurredCB, &opaqueC, &opaqueC, &outCB, vImage_Flags(kvImageDoNotTile)) == kvImageNoError,
              vImageAlphaBlend_Planar8(&cr, &maskC, &blurredCR, &opaqueC, &opaqueC, &outCR, vImage_Flags(kvImageDoNotTile)) == kvImageNoError else {
            return false
        }
        return joinChroma(cb: outCB, cr: outCR, into: &destC)
    }

    private func blur(_ source: vImage_Buffer, into dest: inout vImage_Buffer, name: String, divisor: Int, strength: Double, grade: PortraitGrade) -> Bool {
        var source = source
        let smallWidth = max(8, Int(source.width) / divisor)
        let smallHeight = max(8, Int(source.height) / divisor)
        guard var smallA = scratch.plane("\(name)A", width: smallWidth, height: smallHeight),
              var smallB = scratch.plane("\(name)B", width: smallWidth, height: smallHeight) else { return false }
        let flags = vImage_Flags(kvImageEdgeExtend | kvImageDoNotTile)
        guard vImageScale_Planar8(&source, &smallA, nil, vImage_Flags(kvImageDoNotTile)) == kvImageNoError else { return false }
        let kernel = blurKernel(strength: strength, grade: grade, width: smallWidth, height: smallHeight)
        if kernel >= 3 {
            guard vImageBoxConvolve_Planar8(&smallA, &smallB, nil, 0, 0, kernel, kernel, 0, flags) == kvImageNoError,
                  vImageBoxConvolve_Planar8(&smallB, &smallA, nil, 0, 0, kernel, kernel, 0, flags) == kvImageNoError else { return false }
            return vImageScale_Planar8(&smallA, &dest, nil, vImage_Flags(kvImageDoNotTile)) == kvImageNoError
        }
        return vImageScale_Planar8(&smallA, &dest, nil, vImage_Flags(kvImageDoNotTile)) == kvImageNoError
    }

    private func blurKernel(strength: Double, grade: PortraitGrade, width: Int, height: Int) -> UInt32 {
        let span = grade == .fast ? 12.0 : 20.0
        var radius = Int((1 + strength * span).rounded())
        if radius < 3 { radius = 3 }
        if radius % 2 == 0 { radius += 1 }
        let limit = min(radius, width, height, grade == .fast ? 13 : 21)
        let odd = limit % 2 == 0 ? limit - 1 : limit
        return UInt32(max(odd, 1))
    }

    private func prepare(width: Int, height: Int) {
        guard poolWidth != width || poolHeight != height else { return }
        pool = nil
        poolWidth = width
        poolHeight = height
        scratch.reset()
        hasPreviousMask = false
        maskWidth = 0
        maskHeight = 0
        frameIndex = 0
    }

    private func outputBuffer(width: Int, height: Int) -> CVPixelBuffer? {
        if pool == nil {
            let bufferAttributes: [String: Any] = [
                kCVPixelBufferPixelFormatTypeKey as String: kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange,
                kCVPixelBufferWidthKey as String: width,
                kCVPixelBufferHeightKey as String: height,
                kCVPixelBufferIOSurfacePropertiesKey as String: [:] as [String: Any],
                kCVPixelBufferMetalCompatibilityKey as String: true
            ]
            let poolAttributes: [String: Any] = [
                kCVPixelBufferPoolMinimumBufferCountKey as String: 6
            ]
            var created: CVPixelBufferPool?
            guard CVPixelBufferPoolCreate(kCFAllocatorDefault, poolAttributes as CFDictionary, bufferAttributes as CFDictionary, &created) == kCVReturnSuccess,
                  let created else { return nil }
            pool = created
        }
        guard let pool else { return nil }
        var buffer: CVPixelBuffer?
        guard CVPixelBufferPoolCreatePixelBuffer(kCFAllocatorDefault, pool, &buffer) == kCVReturnSuccess else { return nil }
        return buffer
    }

    private func plane(_ buffer: CVPixelBuffer, index: Int) -> vImage_Buffer? {
        guard let base = CVPixelBufferGetBaseAddressOfPlane(buffer, index) else { return nil }
        return vImage_Buffer(
            data: base,
            height: vImagePixelCount(CVPixelBufferGetHeightOfPlane(buffer, index)),
            width: vImagePixelCount(CVPixelBufferGetWidthOfPlane(buffer, index)),
            rowBytes: CVPixelBufferGetBytesPerRowOfPlane(buffer, index)
        )
    }

    private func blend(current: inout vImage_Buffer, previous: vImage_Buffer, newWeight: Int) {
        let oldWeight = 255 - newWeight
        let width = Int(current.width)
        let height = Int(current.height)
        guard let currentData = current.data, let previousData = previous.data else { return }
        for y in 0..<height {
            let row = currentData.advanced(by: y * current.rowBytes).assumingMemoryBound(to: UInt8.self)
            let prior = previousData.advanced(by: y * previous.rowBytes).assumingMemoryBound(to: UInt8.self)
            for x in 0..<width {
                row[x] = UInt8((Int(row[x]) * newWeight + Int(prior[x]) * oldWeight + 127) / 255)
            }
        }
    }

    private func copy(_ source: vImage_Buffer, into dest: inout vImage_Buffer) -> Bool {
        guard source.width == dest.width, source.height == dest.height, let sourceData = source.data, let destData = dest.data else {
            return false
        }
        let width = Int(source.width)
        let height = Int(source.height)
        for y in 0..<height {
            memcpy(
                destData.advanced(by: y * dest.rowBytes),
                sourceData.advanced(by: y * source.rowBytes),
                width
            )
        }
        return true
    }

    private func splitChroma(_ source: vImage_Buffer, cb: inout vImage_Buffer, cr: inout vImage_Buffer) -> Bool {
        let width = Int(cb.width)
        let height = Int(cb.height)
        guard source.rowBytes >= width * 2, let sourceData = source.data, let cbData = cb.data, let crData = cr.data else { return false }
        for y in 0..<height {
            let row = sourceData.advanced(by: y * source.rowBytes).assumingMemoryBound(to: UInt8.self)
            let cbRow = cbData.advanced(by: y * cb.rowBytes).assumingMemoryBound(to: UInt8.self)
            let crRow = crData.advanced(by: y * cr.rowBytes).assumingMemoryBound(to: UInt8.self)
            for x in 0..<width {
                cbRow[x] = row[x * 2]
                crRow[x] = row[x * 2 + 1]
            }
        }
        return true
    }

    private func joinChroma(cb: vImage_Buffer, cr: vImage_Buffer, into dest: inout vImage_Buffer) -> Bool {
        let width = Int(cb.width)
        let height = Int(cb.height)
        guard let destData = dest.data, let cbData = cb.data, let crData = cr.data else { return false }
        guard dest.rowBytes >= width * 2 else { return false }
        for y in 0..<height {
            let row = destData.advanced(by: y * dest.rowBytes).assumingMemoryBound(to: UInt8.self)
            let cbRow = cbData.advanced(by: y * cb.rowBytes).assumingMemoryBound(to: UInt8.self)
            let crRow = crData.advanced(by: y * cr.rowBytes).assumingMemoryBound(to: UInt8.self)
            for x in 0..<width {
                row[x * 2] = cbRow[x]
                row[x * 2 + 1] = crRow[x]
            }
        }
        return true
    }
}

private final class Scratch {
    private var planes: [String: vImage_Buffer] = [:]

    func plane(_ name: String, width: Int, height: Int, fill: UInt8? = nil) -> vImage_Buffer? {
        guard width > 0, height > 0 else { return nil }
        if let existing = planes[name], Int(existing.width) == width, Int(existing.height) == height {
            return existing
        }
        if let existing = planes.removeValue(forKey: name) {
            existing.data?.deallocate()
        }
        let rowBytes = (width + 15) & ~15
        let count = rowBytes * height
        let pointer = UnsafeMutableRawPointer.allocate(byteCount: count, alignment: 16)
        if let fill {
            pointer.initializeMemory(as: UInt8.self, repeating: fill, count: count)
        }
        let buffer = vImage_Buffer(
            data: pointer,
            height: vImagePixelCount(height),
            width: vImagePixelCount(width),
            rowBytes: rowBytes
        )
        planes[name] = buffer
        return buffer
    }

    func reset() {
        for plane in planes.values {
            plane.data?.deallocate()
        }
        planes.removeAll()
    }

    deinit {
        reset()
    }
}
