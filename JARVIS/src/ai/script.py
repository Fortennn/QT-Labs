"""
JARVIS — Vision / Touchpad engine.

Computer-vision powered virtual touchpad and gesture remote for the JARVIS
desktop app. Drives the host PC via ``pyautogui`` (mouse / keyboard / scroll
/ media keys).
"""

from __future__ import annotations

import argparse
import math
import signal
import sys
import time
from collections import deque

import cv2
import mediapipe as mp
import numpy as np

# pyautogui can fail to import on headless machines. Keep the rest of the
# script usable as a "macro only" mode in that case so we still degrade
# gracefully instead of hard-crashing.
try:
    import pyautogui  # type: ignore
    pyautogui.FAILSAFE = False         # don't kill us if cursor races to (0,0)
    pyautogui.PAUSE = 0                # we manage cadence ourselves
    SCREEN_W, SCREEN_H = pyautogui.size()
    PYAUTOGUI_OK = True
except Exception as exc:  # noqa: BLE001
    PYAUTOGUI_OK = False
    SCREEN_W, SCREEN_H = 1920, 1080
    print(f"[JARVIS VISION] ⚠ pyautogui unavailable ({exc}); macro-only mode.",
          flush=True)


# ─── stdio plumbing ──────────────────────────────────────────────────────────────────────
try:
    sys.stdout.reconfigure(encoding="utf-8", errors="replace", line_buffering=True)
    sys.stderr.reconfigure(encoding="utf-8", errors="replace", line_buffering=True)
except Exception:
    pass


def log(msg: str) -> None:
    """Single line-buffered logger; the C++ side parses these in real time."""
    print(f"[JARVIS VISION] {msg}", flush=True)


# ─── Args ──────────────────────────────────────────────────────────────────────
parser = argparse.ArgumentParser()
parser.add_argument("--port", type=int, default=17320,
                    help="JarvisHttpServer port (default matches MainWindow.cpp).")
parser.add_argument("--pin", type=str, default="",
                    help="Optional X-JARVIS-PIN auth header.")
parser.add_argument("--camera", type=int, default=0,
                    help="OpenCV camera index (default 0).")
parser.add_argument("--mirror", action="store_true", default=True,
                    help="Mirror the video horizontally (default true).")
parser.add_argument("--no-mirror", action="store_false", dest="mirror")
parser.add_argument("--no-touchpad", action="store_true",
                    help="Start in MACRO-only mode (no mouse control).")
args = parser.parse_args()


# ─── Graceful shutdown ───────────────────────────────────────────────────────
_should_exit = False


def _stop(_signum=None, _frame=None) -> None:
    global _should_exit
    _should_exit = True


# QProcess::terminate() sends SIGTERM on POSIX and a console CTRL_BREAK on
# Windows; both eventually become signals we can listen for here.
for _sig in (signal.SIGINT, signal.SIGTERM):
    try:
        signal.signal(_sig, _stop)
    except (ValueError, OSError):
        pass


# ─── Landmark constants ────────────────────────────────────────────────
FINGER_TIPS = [8, 12, 16, 20]
FINGER_PIP  = [6, 10, 14, 18]

WRIST       = 0
THUMB_TIP   = 4
INDEX_TIP   = 8
MIDDLE_TIP  = 12
RING_TIP    = 16
PINKY_TIP   = 20


def dist(a, b) -> float:
    return math.hypot(a.x - b.x, a.y - b.y)


def finger_up(lm, tip: int, pip: int) -> bool:
    return lm[tip].y < lm[pip].y


def thumb_extended(lm, hand_label: str) -> bool:
    tip = lm[THUMB_TIP]
    base = lm[2]
    if hand_label == "Right":
        return tip.x < base.x
    return tip.x > base.x


# ─── Gesture classifier ────────────────────────────────────────────────
def classify_gesture(lm, hand_label: str) -> str | None:
    fingers = [finger_up(lm, t, p) for t, p in zip(FINGER_TIPS, FINGER_PIP)]
    thumb = thumb_extended(lm, hand_label)
    count = sum(fingers)
    idx, mid, rng, pnk = fingers

    if thumb and count == 0:
        return "THUMBS_UP"
    if not thumb and count == 0:
        return "FIST"
    if count == 4 and thumb:
        return "OPEN_HAND"
    if idx and mid and not rng and not pnk:
        return "PEACE"
    if idx and not mid and not rng and not pnk:
        return "POINT"
    return None


# ─── Gesture Labels in Ukrainian ────────────────────────────────────────────────
GESTURE_LABELS = {
    "OPEN_HAND": ("Відкрита долоня", "рух курсору"),
    "POINT":     ("Вказівний палець", "рух курсору"),
    "FIST":      ("Кулак",           "лівий клік / утримання"),
    "THUMBS_UP": ("Клас (вгору)",    "правий клік"),
    "PEACE":     ("Жест Peace",      "прокрутка (скрол)"),
}


# ─── Palette ──────────────────────────────────────────────────────────────────────
BG_PANEL        = (12, 14, 18)
ACCENT          = (255, 198, 0)
ACCENT_SOFT     = (255, 230, 110)
GOOD            = (110, 230, 130)
WARN            = (90, 130, 255)
TEXT            = (235, 240, 246)
TEXT_DIM        = (160, 168, 180)
TEXT_FAINT      = (110, 118, 130)
JOINT_COLOR     = (60, 220, 240)
ACTIVE_JOINT    = (90, 200, 255)
JOINT_RADIUS    = 6
BONE_THICKNESS  = 3

FINGER_GROUPS = {
    "THUMB":  [(0, 1), (1, 2), (2, 3), (3, 4)],
    "INDEX":  [(5, 6), (6, 7), (7, 8)],
    "MIDDLE": [(9, 10), (10, 11), (11, 12)],
    "RING":   [(13, 14), (14, 15), (15, 16)],
    "PINKY":  [(17, 18), (18, 19), (19, 20)],
    "PALM":   [(0, 5), (5, 9), (9, 13), (13, 17), (0, 17)],
}
FINGER_COLORS = {
    "THUMB":  (90, 110, 255),
    "INDEX":  (255, 200, 60),
    "MIDDLE": (90, 230, 150),
    "RING":   (80, 200, 255),
    "PINKY":  (220, 110, 255),
    "PALM":   (130, 138, 148),
}


# ─── Drawing helpers ──────────────────────────────────────────────────
def rounded_rect(img, p1, p2, color, radius=14, thickness=-1, alpha=1.0):
    """Filled rounded rectangle composited with optional alpha."""
    x1, y1 = p1
    x2, y2 = p2
    overlay = img.copy() if alpha < 1.0 else img
    cv2.rectangle(overlay, (x1 + radius, y1), (x2 - radius, y2), color, thickness, cv2.LINE_AA)
    cv2.rectangle(overlay, (x1, y1 + radius), (x2, y2 - radius), color, thickness, cv2.LINE_AA)
    cv2.circle(overlay, (x1 + radius, y1 + radius), radius, color, thickness, cv2.LINE_AA)
    cv2.circle(overlay, (x2 - radius, y1 + radius), radius, color, thickness, cv2.LINE_AA)
    cv2.circle(overlay, (x1 + radius, y2 - radius), radius, color, thickness, cv2.LINE_AA)
    cv2.circle(overlay, (x2 - radius, y2 - radius), radius, color, thickness, cv2.LINE_AA)
    if alpha < 1.0:
        cv2.addWeighted(overlay, alpha, img, 1.0 - alpha, 0, img)


def shadowed_text(img, text, org, scale, color, thickness=1):
    x, y = org
    cv2.putText(img, text, (x + 1, y + 1), cv2.FONT_HERSHEY_DUPLEX,
                scale, (0, 0, 0), thickness + 1, cv2.LINE_AA)
    cv2.putText(img, text, org, cv2.FONT_HERSHEY_DUPLEX,
                scale, color, thickness, cv2.LINE_AA)


def draw_skeleton(frame, lm_list, h, w, active=False):
    pts = [(int(lm.x * w), int(lm.y * h)) for lm in lm_list]
    for group, connections in FINGER_GROUPS.items():
        color = ACCENT if active else FINGER_COLORS[group]
        for (a, b) in connections:
            cv2.line(frame, pts[a], pts[b], color, BONE_THICKNESS, cv2.LINE_AA)
    for pt in pts:
        cv2.circle(frame, pt, JOINT_RADIUS,
                   ACTIVE_JOINT if active else JOINT_COLOR, -1, cv2.LINE_AA)
        cv2.circle(frame, pt, JOINT_RADIUS + 2, (0, 0, 0), 1, cv2.LINE_AA)


def draw_topbar(frame, mode_label, gesture_label, fps):
    h, w = frame.shape[:2]
    bar_h = 56
    rounded_rect(frame, (12, 12), (w - 12, 12 + bar_h),
                 BG_PANEL, radius=18, alpha=0.78)
    # Status dot
    cv2.circle(frame, (38, 12 + bar_h // 2), 8, GOOD, -1, cv2.LINE_AA)
    cv2.circle(frame, (38, 12 + bar_h // 2), 12, GOOD, 1, cv2.LINE_AA)
    shadowed_text(frame, "JARVIS  VISION",
                  (60, 12 + bar_h // 2 + 7), 0.70, TEXT, 1)
    # Mode pill
    pill_x1, pill_y1 = 250, 12 + 12
    pill_x2, pill_y2 = 250 + 200, 12 + bar_h - 12
    rounded_rect(frame, (pill_x1, pill_y1), (pill_x2, pill_y2),
                 (40, 44, 54), radius=14, alpha=0.95)
    shadowed_text(frame, mode_label,
                  (pill_x1 + 14, pill_y1 + 22), 0.55, ACCENT_SOFT, 1)
    # Gesture readout
    if gesture_label:
        shadowed_text(frame, gesture_label,
                      (pill_x2 + 20, 12 + bar_h // 2 + 7), 0.62, TEXT, 1)
    # FPS at the far right
    fps_text = f"{fps:5.1f} FPS"
    shadowed_text(frame, fps_text,
                  (w - 120, 12 + bar_h // 2 + 7), 0.55, TEXT_DIM, 1)


def draw_legend(frame, mode):
    h, w = frame.shape[:2]
    legend_w = 380
    legend_h = 32 + len(GESTURE_LABELS) * 22 + 20
    x1, y1 = w - legend_w - 16, h - legend_h - 16
    x2, y2 = w - 16, h - 16
    rounded_rect(frame, (x1, y1), (x2, y2), BG_PANEL, radius=16, alpha=0.78)
    shadowed_text(frame, "ЛЕГЕНДА ЖЕСТІВ", (x1 + 16, y1 + 26), 0.50, ACCENT_SOFT, 1)
    for i, (gname, (head, tail)) in enumerate(GESTURE_LABELS.items()):
        row_y = y1 + 50 + i * 22
        shadowed_text(frame, head, (x1 + 16, row_y), 0.44, TEXT, 1)
        shadowed_text(frame, "→", (x1 + 160, row_y), 0.44, TEXT_FAINT, 1)
        shadowed_text(frame, tail, (x1 + 180, row_y), 0.44, TEXT_DIM, 1)
    # Mode hint
    hint = "H: сховати легенду    Q / Esc: вихід"
    shadowed_text(frame, hint, (16, h - 18), 0.42, TEXT_FAINT, 1)


def draw_cursor_overlay(frame, fingertip_px, click=False):
    """Crosshair drawn at the active fingertip in TOUCHPAD mode."""
    x, y = fingertip_px
    color = GOOD if click else ACCENT
    cv2.circle(frame, (x, y), 22, color, 2, cv2.LINE_AA)
    cv2.line(frame, (x - 30, y), (x - 10, y), color, 2, cv2.LINE_AA)
    cv2.line(frame, (x + 10, y), (x + 30, y), color, 2, cv2.LINE_AA)
    cv2.line(frame, (x, y - 30), (x, y - 10), color, 2, cv2.LINE_AA)
    cv2.line(frame, (x, y + 10), (x, y + 30), color, 2, cv2.LINE_AA)


# ─── Touchpad state ──────────────────────────────────────────────────
class Touchpad:
    """Translates landmark motion into mouse / scroll / click events.

    Cursor movement:
        Middle finger MCP knuckle (lm[9]) -> cursor coordinates.
        This provides a highly stable point that does not jump when fingers
        clench or open.

    Left Click / Drag : Hand clenches into a Fist (FIST).
    Right Click       : Thumbs up (THUMBS_UP).
    Scroll            : PEACE gesture. Vertical movement scrolls up/down.
    """

    SMOOTH_ALPHA       = 0.35      # 0..1, higher = snappier cursor.
    SCROLL_GAIN        = 800.0
    ACTIVE_BORDER_PCT  = 0.12      # ignore outer 12% so wrists at frame edge
                                   # don't slam the cursor into the corner.

    def __init__(self) -> None:
        self.smoothed_x: float | None = None
        self.smoothed_y: float | None = None
        self.last_scroll_y: float | None = None
        self.left_down = False
        self.right_down = False

    # ── helpers ──
    def map_to_screen(self, nx: float, ny: float) -> tuple[int, int]:
        # Rescale away from the active-border buffer.
        b = self.ACTIVE_BORDER_PCT
        nx = (np.clip(nx, b, 1 - b) - b) / max(1 - 2 * b, 1e-6)
        ny = (np.clip(ny, b, 1 - b) - b) / max(1 - 2 * b, 1e-6)
        return int(nx * SCREEN_W), int(ny * SCREEN_H)

    def smooth(self, nx: float, ny: float) -> tuple[float, float]:
        a = self.SMOOTH_ALPHA
        if self.smoothed_x is None:
            self.smoothed_x, self.smoothed_y = nx, ny
        else:
            self.smoothed_x = a * nx + (1 - a) * self.smoothed_x
            self.smoothed_y = a * ny + (1 - a) * self.smoothed_y
        return self.smoothed_x, self.smoothed_y

    # ── primary update ──
    def update(self, lm, gesture: str | None) -> dict:
        """Drive mouse / scroll given a single hand's landmarks."""
        hint: dict = {"cursor_px": None, "click": False, "scroll": 0}
        if not PYAUTOGUI_OK:
            return hint

        # Control point: base of the middle finger (knuckle)
        ref_pt = lm[9]

        # ── PEACE → scroll ──
        if gesture == "PEACE":
            cy = ref_pt.y
            if self.last_scroll_y is None:
                self.last_scroll_y = cy
            dy = self.last_scroll_y - cy
            self.last_scroll_y = cy
            steps = int(dy * self.SCROLL_GAIN)
            if abs(steps) >= 1:
                pyautogui.scroll(steps)
                hint["scroll"] = steps
            self._release_clicks()
            return hint
        self.last_scroll_y = None

        # ── Normal cursor movement (Fist, Thumbs Up, or Open/Point) ──
        sx, sy = self.smooth(ref_pt.x, ref_pt.y)
        px, py = self.map_to_screen(sx, sy)
        pyautogui.moveTo(px, py, _pause=False)
        hint["cursor_px"] = (ref_pt.x, ref_pt.y)

        # Handle mouse clicks
        if gesture == "FIST":
            if not self.left_down:
                pyautogui.mouseDown(button="left", _pause=False)
                self.left_down = True
            hint["click"] = True
            if self.right_down:
                pyautogui.mouseUp(button="right", _pause=False)
                self.right_down = False
        elif gesture == "THUMBS_UP":
            if not self.right_down:
                pyautogui.mouseDown(button="right", _pause=False)
                self.right_down = True
            hint["click"] = True
            if self.left_down:
                pyautogui.mouseUp(button="left", _pause=False)
                self.left_down = False
        else:
            self._release_clicks()

        return hint

    def _release_clicks(self) -> None:
        if not PYAUTOGUI_OK:
            return
        if self.left_down:
            pyautogui.mouseUp(button="left", _pause=False)
            self.left_down = False
        if self.right_down:
            pyautogui.mouseUp(button="right", _pause=False)
            self.right_down = False

    def shutdown(self) -> None:
        self._release_clicks()


# ─── Init mediapipe + camera ───────────────────────────────────────────────────────
log("Initialising MediaPipe Hand Tracking…")
mp_hands = mp.solutions.hands
hands_detector = mp_hands.Hands(
    static_image_mode=False,
    max_num_hands=2,
    min_detection_confidence=0.75,
    min_tracking_confidence=0.55,
)

log(f"Opening camera (index={args.camera})…")
# On Windows prefer DirectShow (CAP_DSHOW) over the default MSMF backend.
_cam_backends: list[int] = (
    [cv2.CAP_DSHOW, cv2.CAP_MSMF, cv2.CAP_ANY]
    if sys.platform.startswith("win") else
    [cv2.CAP_V4L2, cv2.CAP_ANY]
)
cap: cv2.VideoCapture | None = None
for _backend in _cam_backends:
    log(f"  trying backend {_backend}…")
    _cap = cv2.VideoCapture(args.camera + _backend * 0, _backend)
    if _cap.isOpened():
        cap = _cap
        log(f"Camera opened OK (backend={_backend}).")
        break
    _cap.release()

if cap is None or not cap.isOpened():
    log("ERROR: camera not found — жоден бекенд не зміг відкрити камеру. Перевір, що інша програма не зайняла камеру.")
    sys.exit(1)

cap.set(cv2.CAP_PROP_FRAME_WIDTH,  1280)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)

WINDOW = "JARVIS Vision"
cv2.namedWindow(WINDOW, cv2.WINDOW_NORMAL)
cv2.resizeWindow(WINDOW, 1280, 720)

touchpad_mode = not args.no_touchpad
show_legend   = True
fullscreen    = False
touchpad      = Touchpad()

# FPS measured with a rolling window so it doesn't jitter.
fps_window: deque[float] = deque(maxlen=30)
prev_t = time.time()

log(f"Touchpad mode: {'ON' if touchpad_mode else 'OFF'} | "
    f"screen={SCREEN_W}x{SCREEN_H} | "
    f"pyautogui={'ok' if PYAUTOGUI_OK else 'unavailable'}")
log("JARVIS Vision active. Show your hands! Press 'q' to quit.")


# ─── Main loop ─────────────────────────────────────────────────────────
_read_fail_count = 0
_MAX_READ_FAILS   = 60  # ~2 s at 30 fps before we give up

while cap.isOpened() and not _should_exit:
    ok, frame = cap.read()
    if not ok:
        _read_fail_count += 1
        if _read_fail_count >= _MAX_READ_FAILS:
            log("ERROR: camera stopped delivering frames — виходжу.")
            break
        time.sleep(0.033)  # avoid busy-spin while waiting for camera to recover
        continue
    _read_fail_count = 0
    if args.mirror:
        frame = cv2.flip(frame, 1)
    h, w = frame.shape[:2]

    # FPS
    now = time.time()
    fps_window.append(1.0 / max(now - prev_t, 1e-6))
    prev_t = now
    fps = sum(fps_window) / len(fps_window)

    rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    results = hands_detector.process(rgb)

    best_gesture: str | None = None
    cursor_px: tuple[float, float] | None = None
    click_now = False

    if results.multi_hand_landmarks and results.multi_handedness:
        for hand_lm, hand_info in zip(results.multi_hand_landmarks,
                                      results.multi_handedness):
            label = hand_info.classification[0].label
            lm = hand_lm.landmark
            gesture = classify_gesture(lm, label)
            if gesture and best_gesture is None:
                best_gesture = gesture

            # Touchpad runs off the first hand to avoid mouse-jitter.
            if touchpad_mode:
                hint = touchpad.update(lm, gesture)
                if hint["cursor_px"] is not None:
                    cursor_px = hint["cursor_px"]
                if hint["click"]:
                    click_now = True

            active = gesture is not None
            draw_skeleton(frame, lm, h, w, active=active)
    else:
        touchpad._release_clicks()
        touchpad.smoothed_x = touchpad.smoothed_y = None

    # ── HUD ──────────────────────────────────────────────────────────────────────
    mode_label = "ТАЧПАД"
    head_tail = GESTURE_LABELS.get(best_gesture) if best_gesture else None
    gesture_label = f"{head_tail[0]} · {head_tail[1]}" if head_tail else ""
    draw_topbar(frame, mode_label, gesture_label, fps)
    if show_legend:
        draw_legend(frame, mode_label)
    if touchpad_mode and cursor_px is not None:
        draw_cursor_overlay(frame,
                             (int(cursor_px[0] * w), int(cursor_px[1] * h)),
                             click=click_now)

    cv2.imshow(WINDOW, frame)

    key = cv2.waitKey(1) & 0xFF
    if key in (ord('q'), 27):
        break
    if key == ord('h'):
        show_legend = not show_legend
    if key == ord('f'):
        fullscreen = not fullscreen
        cv2.setWindowProperty(
            WINDOW, cv2.WND_PROP_FULLSCREEN,
            cv2.WINDOW_FULLSCREEN if fullscreen else cv2.WINDOW_NORMAL)

    try:
        if cv2.getWindowProperty(WINDOW, cv2.WND_PROP_VISIBLE) < 1:
            break
    except cv2.error:
        break


# ─── Shutdown ──────────────────────────────────────────────────────────
touchpad.shutdown()
cap.release()
cv2.destroyAllWindows()
log("Vision engine closed.")
