# VCamdroid visual system

Operator surfaces follow **Cleanly · A · Graphite**. This file is the token table. Adapters must not invent a second palette.

| Adapter | Where |
|---|---|
| Windows shell | `windows/src/ui/panel_shell.h` |
| Camera desk `/panel` | `ios/VCamdroid/DroidCam/ControlPanelPage.swift` |
| iPhone studio | `ios/VCamdroid/Theme/StudioTheme.swift` |

Copy Cleanly, do not restyle Cleanly. Pigment stays in capsules and the live preview, never in chrome.

## Scene

A live camera desk in a dim room. Light paper is Cleanly’s original look; this product uses Graphite.

## Tokens

```
paper          #161618
ink            #f5f5f7
ink-2          #c7c7cc
ink-3          #a8a8af
on-ink         #161618
surface        #202024
surface-2      #1b1b1e
raised         #2a2a2e
raised-2       #36363c
line           rgba(255,255,255,0.08)
hair           rgba(255,255,255,0.08)
prism-a        #6d7bff
prism-b        #a77bff
prism-c        #ff8fc8
mint           #30d158
amber          #ffb547
danger         #ff6b81
ease           cubic-bezier(0.2, 0.8, 0.2, 1)
radius         28px
rhythm         18px
titlebar       44px
nav            236px
display        Unbounded (iOS: rounded SF)
ui             Onest (iOS: SF)
mono           JetBrains Mono (iOS: monospacedDigit)
```

## Rules

- Primary actions are Graphite primary (pale fill, dark label), not prism.
- Selected chips use a prism ring or prism fill. Semantic warn uses amber. Live uses mint. Stop uses danger.
- One ease. Press scale is `0.98`. Arrivals use a 12px blur-rise, once, not on every preview frame.
- Section gap is 18px. Cards are 28px. Controls are capsules.
- Hairline on raised Graphite surfaces: `inset 0 1px 0 hair`.
- Do not ship Flow, bubble physics, or a fourth hue on the ribbon.
