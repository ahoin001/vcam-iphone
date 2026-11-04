# Guide photos

The Lighting tab of the Guide (on the camera desk and in VCamPanel) uses these photos. Each one was resized to 800 px on the long edge and recompressed. The resized copies keep their original licenses.

The lighting advice is adapted from Senpai, [“The Secret to Lighting Your Videos and Live Streams” (Streaming MasterClass Ep. 9)](https://www.youtube.com/watch?v=mCxWTtDH-f4). No frames from that video are used.

| File | Shows | Author | License | Source |
| --- | --- | --- | --- | --- |
| `guide-diffusion.jpg` | Hard light vs. a softbox | Alex (MorrowLess) | [CC BY-SA 2.0](https://creativecommons.org/licenses/by-sa/2.0/) | [Wikimedia Commons](https://commons.wikimedia.org/wiki/File:Portraits_with_and_without_softbox_lighting,_by_MorrowLess.jpg) |
| `guide-key-fill.jpg` | Key at 10–11 o’clock with a darker shadow side | Jacopo188 | [CC BY-SA 3.0](https://creativecommons.org/licenses/by-sa/3.0/) | [Wikimedia Commons](https://commons.wikimedia.org/wiki/File:Rembrandt_Lighting.JPG) |
| `guide-edge.jpg` | Edge (rim) light alone | japrea | [CC BY-SA 2.0](https://creativecommons.org/licenses/by-sa/2.0/) | [Flickr](https://www.flickr.com/photos/58455346@N00/4724038449) |
| `guide-butterfly.jpg` | Key above and in front | Jay DeFehr | [CC BY-SA 2.0](https://creativecommons.org/licenses/by-sa/2.0/) | [Flickr](https://www.flickr.com/photos/50841206@N02/14962847439) |
| `guide-short.jpg` | Short lighting | Kieran Clarke | [CC BY 2.0](https://creativecommons.org/licenses/by/2.0/) | [Flickr](https://www.flickr.com/photos/71835142@N08/8109583619) |
| `guide-window.jpg` | Silhouette against a bright window | Thomas Leuthard | [CC BY 2.0](https://creativecommons.org/licenses/by/2.0/) | [Flickr](https://www.flickr.com/photos/41346951@N05/15629546536) |

## Where they live

- `ios/VCamdroid/Guide/guide-*.jpg` is the only copy. The iPhone app bundles the folder and serves each photo at `/guide/<name>.jpg`; the desk loads them when the Guide opens.
- `windows/src/guide_photos.h` embeds the same files in VCamPanel so its Guide works before a phone connects. Run `python3 scripts/embed-guide-photos.py` after changing a photo.
- The credit line at the bottom of the Lighting tab must match this table.
