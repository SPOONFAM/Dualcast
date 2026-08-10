# Dualcast

Dualcast is a native OBS Studio plugin for independently streaming the current OBS program mix to YouTube Live and TikTok LIVE-compatible RTMP/RTMPS ingest destinations.

This repository currently implements the production foundation and first usable output path:

- A dockable `Docks → Dualcast` Qt/OBS frontend panel.
- Independent YouTube and TikTok manual RTMP/RTMPS destinations.
- Independent OBS `rtmp_output`, service, video encoder, and AAC audio encoder objects per destination.
- Per-destination resolution, FPS, bitrate, audio bitrate, keyframe interval, encoder preference, and reconnect settings.
- Independent start/stop behavior and OBS output reconnect handling; one platform stopping does not stop the other.
- Output health counters from OBS, bitrate estimation, capacity warnings, and preflight checks.
- Windows DPAPI protection for stream keys and OAuth tokens. Secrets never enter logs or the saved OBS profile in plaintext.
- Provider interfaces that keep YouTube account/OAuth work separate from TikTok LIVE ingest. TikTok OAuth identity is not treated as a LIVE stream-key capability.
- A real Google installed-app OAuth flow when `DUALCAST_GOOGLE_CLIENT_ID` is configured at build time.

## Build on Windows

Install the OBS Studio development files or build tree matching the target OBS version, CMake 3.28+, Qt 6 development files, and Visual Studio 2022. The normal OBS runtime install does not include the CMake package files needed to compile plugins.

```powershell
cmake -S . -B build -DDUALCAST_OBS_DIR="C:\path\to\obs-development-prefix" -DDUALCAST_GOOGLE_CLIENT_ID="your-installed-app-client-id"
cmake --build build --config RelWithDebInfo
ctest --test-dir build -C RelWithDebInfo --output-on-failure
```

Install the resulting `Dualcast.dll` under the OBS plugin directory and copy `resources` under OBS's `data/obs-plugins/Dualcast` directory. Use an OBS development package or the official OBS plugin-template dependency workflow when producing a release package.

## Platform and API boundaries

The plugin uses only documented OBS frontend/core/output/encoder/service APIs. The public OBS API does not expose encoder fan-out to multiple outputs; the internal `obs_encoder_add_output` symbol is deliberately not used. Compatible destinations are detected and reported, but Dualcast creates independent encoder contexts until OBS exposes a supported sharing API.

The vertical TikTok layout model and editor are intentionally not wired to a live output yet. A different aspect ratio requires a supported compositor/video pipeline; changing the normal OBS canvas or mutating the user's scenes would violate Dualcast's integration contract. The editor currently makes this boundary explicit instead of presenting an unsafe or nonfunctional live-layout control.

TikTok LIVE credentials must be supplied by the user from an officially authorized TikTok LIVE workflow. Dualcast never scrapes TikTok, automates a browser to obtain hidden keys, uses session cookies, or bypasses LIVE eligibility.

## Security

On Windows, stream keys and OAuth tokens are encrypted with DPAPI and stored only as encrypted blobs in the user's native settings store. Passwords are never collected. Other platforms currently fail closed until their native keychain/secret-service adapters are implemented.

## License

GPL-2.0-or-later, matching the OBS plugin ecosystem.
