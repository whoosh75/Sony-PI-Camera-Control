# Quick shortcuts to run CCU (one-line) ✅

Use these from the repo root:

- Start daemon (default port 5555):

  SONY_PASS="Password1" ./start_ccu.sh

- Run diagnostic (enumerate cameras and print fingerprints):

  SONY_PASS="Password1" ./start_ccu.sh --diag

- Select camera by index (1-based):

  SONY_PASS="Password1" SONY_CAMERA_INDEX="2" ./start_ccu.sh

- Select camera by MAC (case-insensitive):

  SONY_PASS="Password1" SONY_CAMERA_MAC="50:26:EF:B8:3F:2C" ./start_ccu.sh

- Direct IP fallback (if enumeration fails):

  SONY_PASS="Password1" SONY_CAMERA_IP="192.168.33.94" ./start_ccu.sh

Notes:
- You either set `SONY_PASS` inline as above, or `export SONY_PASS="Password1"` then run `./start_ccu.sh`.
- If you prefer running from the `build/` directory, use `./run_ccu.sh` there: `SONY_PASS=... ./run_ccu.sh`.

Important: fingerprint acceptance & connect behavior
- For Ethernet cameras the SDK requires an SSH handshake (password-only). The daemon prints the camera fingerprint and will *not* auto-accept it unless you explicitly set:

  SONY_ACCEPT_FINGERPRINT=1

  Example: SONY_PASS="Password1" SONY_ACCEPT_FINGERPRINT=1 SONY_CAMERA_IP="192.168.33.94" ./start_ccu.sh

- The daemon will retry Connect up to 5 times with exponential backoff and it will log connection error categories to help debugging.

Important: MPC-2610 record start/stop
- MPC-2610 uses a Down -> Up toggle to start/stop recording.
- Record start can return 0x8402 even when the camera starts recording.
- To treat 0x8402 as success in the daemon, set:

  SONY_ALLOW_INVALID_CALLED=1
