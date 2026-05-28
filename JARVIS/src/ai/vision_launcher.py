"""
JARVIS — Vision / Touchpad launcher.

This file is the *only* Python entry point that the C++ side (QProcess in
``MainWindow::toggleCameraMode``) ever invokes directly.  It runs under
whatever host Python the user already has installed (the C++ side picks
the newest available via the ``py`` launcher), and is responsible for:

    1. Creating a project-local virtual environment under
       ``<AppLocalDataLocation>/jarvis-vision/.venv`` on first run.

    2. Installing / upgrading every dependency listed in
       ``requirements.txt`` *inside that venv* — never into the user's
       system Python.

    3. Re-spawning ``script.py`` with the venv's interpreter and inheriting
       its stdio so the Qt GUI's bubble logger sees a single, continuous
       stream of messages.

The whole point is that the C++ host process *cannot* see the user's
pip-installed packages when run through QProcess on Windows (see the
extended root-cause comment in ``MainWindow::toggleCameraMode``).  Instead
of fighting that, we just ship our own isolated venv and never depend on
the user's PATH layout being sane.

Every message printed here is prefixed with ``[VENV]`` / ``[JARVIS VISION]``
so the GUI can colour them differently if it wants.  All output is flushed
line-by-line (``sys.stdout.reconfigure(line_buffering=True)``) so QProcess's
``readyReadStandardOutput`` fires on every line, not at process exit.
"""

from __future__ import annotations

import argparse
import os
import sys
import subprocess
from pathlib import Path


# ── Stdio: line-buffered + UTF-8 so the Qt bubble logger gets readable output.
try:
    sys.stdout.reconfigure(encoding="utf-8", errors="replace", line_buffering=True)
    sys.stderr.reconfigure(encoding="utf-8", errors="replace", line_buffering=True)
except Exception:
    pass


HERE = Path(__file__).resolve().parent
SCRIPT_PY = HERE / "script.py"
REQUIREMENTS = HERE / "requirements.txt"


def log(msg: str) -> None:
    print(f"[VENV] {msg}", flush=True)


# ── venv layout ─────────────────────────────────────────────────────────────
def venv_root(custom: str | None) -> Path:
    """Choose where the JARVIS vision venv lives.

    Priority:
      1. ``--venv <path>`` from the command line (used by the C++ side so
         the venv ends up next to the .exe's appdata folder, matching
         QStandardPaths::AppLocalDataLocation).
      2. ``%LOCALAPPDATA%\\JARVIS\\jarvis-vision`` on Windows.
      3. ``~/.local/share/JARVIS/jarvis-vision`` on Linux / macOS.
    """
    if custom:
        return Path(custom).expanduser().resolve()
    if sys.platform.startswith("win"):
        base = os.environ.get("LOCALAPPDATA") or os.environ.get("APPDATA")
        if base:
            return Path(base) / "JARVIS" / "jarvis-vision"
    xdg = os.environ.get("XDG_DATA_HOME") or str(Path.home() / ".local" / "share")
    return Path(xdg) / "JARVIS" / "jarvis-vision"


def venv_python(root: Path) -> Path:
    if sys.platform.startswith("win"):
        return root / ".venv" / "Scripts" / "python.exe"
    return root / ".venv" / "bin" / "python"


def venv_pip_marker(root: Path, is_portable: bool = False) -> Path:
    """Marker file written after a successful ``pip install``.  Re-runs are
    skipped while it matches the current requirements.txt mtime so we don't
    re-install on every camera toggle."""
    if is_portable:
        return root / "portable-python" / ".jarvis-deps-installed"
    return root / ".venv" / ".jarvis-deps-installed"


# ── Bootstrap ───────────────────────────────────────────────────────────────
def ensure_venv(root: Path) -> Path:
    """Create the venv if missing, return the path to its python.exe."""
    py = venv_python(root)
    if py.exists():
        # Check if the venv's Python version matches the host Python version
        try:
            res = subprocess.run(
                [str(py), "-c", "import sys; print(sys.version_info[:2])"],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                check=True
            )
            host_ver = f"({sys.version_info.major}, {sys.version_info.minor})"
            if host_ver in res.stdout:
                return py
            else:
                log(f"Версія Python у віртуальному оточенні ({res.stdout.strip()}) не збігається з хостом {host_ver}. Перестворюю оточення...")
        except Exception:
            log("Не вдалося перевірити версію Python у віртуальному оточенні. Перестворюю оточення...")

        # Recreate: delete the old venv
        import shutil
        try:
            shutil.rmtree(root / ".venv")
        except Exception as e:
            log(f"Попередження: не вдалося видалити старе віртуальне оточення: {e}")

    root.mkdir(parents=True, exist_ok=True)
    log(f"Створюю віртуальне оточення: {root / '.venv'}")
    # ``--upgrade-deps`` lazily pulls a recent pip; harmless if the host Python
    # is too old for the flag — fall back to a plain venv.
    try:
        subprocess.check_call(
            [sys.executable, "-m", "venv", "--upgrade-deps", str(root / ".venv")]
        )
    except subprocess.CalledProcessError:
        subprocess.check_call(
            [sys.executable, "-m", "venv", str(root / ".venv")]
        )
    if not py.exists():
        raise RuntimeError(
            f"venv created but interpreter not found at {py}. "
            "Перевір, що в host Python увімкнено модуль `venv`."
        )
    return py


def setup_portable_python(root: Path) -> Path:
    """Download and set up a portable Python 3.11.9 distribution for Windows."""
    portable_dir = root / "portable-python"
    py_exe = portable_dir / "python.exe"
    
    if py_exe.exists():
        # Verify it runs and is indeed 3.11
        try:
            res = subprocess.run(
                [str(py_exe), "-c", "import sys; print(sys.version_info[:2])"],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                check=True
            )
            if "(3, 11)" in res.stdout:
                return py_exe
        except Exception:
            pass

    portable_dir.mkdir(parents=True, exist_ok=True)
    
    # 1. Download official Python 3.11.9 embeddable zip
    is_64bit = sys.maxsize > 2**32
    arch = "amd64" if is_64bit else "win32"
    zip_url = f"https://www.python.org/ftp/python/3.11.9/python-3.11.9-embed-{arch}.zip"
    zip_path = portable_dir / f"python-3.11.9-embed-{arch}.zip"
    
    log(f"Завантажую портативний Python 3.11.9 ({arch}) для роботи камери...")
    import urllib.request
    try:
        urllib.request.urlretrieve(zip_url, zip_path)
    except Exception as e:
        raise RuntimeError(f"Не вдалося завантажити портативний Python з {zip_url}: {e}")
        
    # 2. Extract zip
    log("Розпаковую портативний Python...")
    import zipfile
    try:
        with zipfile.ZipFile(zip_path, 'r') as zip_ref:
            zip_ref.extractall(portable_dir)
    except Exception as e:
        raise RuntimeError(f"Не вдалося розпакувати файл {zip_path}: {e}")
    finally:
        if zip_path.exists():
            try:
                zip_path.unlink()
            except OSError:
                pass
                
    # 3. Enable site-packages by modifying python311._pth
    pth_file = portable_dir / "python311._pth"
    if pth_file.exists():
        content = pth_file.read_text(encoding="utf-8")
        # uncomment 'import site'
        content = content.replace("#import site", "import site")
        if "import site" not in content:
            content += "\nimport site\n"
        pth_file.write_text(content, encoding="utf-8")

    # 4. Bootstrap pip using get-pip.py
    log("Встановлюю менеджер пакетів pip у портативне середовище...")
    get_pip_url = "https://bootstrap.pypa.io/get-pip.py"
    get_pip_path = portable_dir / "get-pip.py"
    try:
        urllib.request.urlretrieve(get_pip_url, get_pip_path)
    except Exception as e:
        raise RuntimeError(f"Не вдалося завантажити get-pip.py з {get_pip_url}: {e}")

    try:
        subprocess.run([str(py_exe), str(get_pip_path), "--no-warn-script-location"], check=True)
    except subprocess.CalledProcessError as e:
        raise RuntimeError(f"Не вдалося встановити pip: {e}")
    finally:
        if get_pip_path.exists():
            try:
                get_pip_path.unlink()
            except OSError:
                pass

    log("Портативний Python 3.11.9 успішно налаштований.")
    return py_exe


def ensure_deps(py: Path, root: Path, is_portable: bool = False) -> None:
    """Install/upgrade requirements.txt inside the venv or portable-python (idempotent)."""
    marker = venv_pip_marker(root, is_portable)
    req_mtime = REQUIREMENTS.stat().st_mtime if REQUIREMENTS.exists() else 0.0

    if marker.exists():
        try:
            stamped = float(marker.read_text(encoding="utf-8").strip())
            if abs(stamped - req_mtime) < 1.0:
                log("Залежності вже встановлені, пропускаю pip install.")
                return
        except (ValueError, OSError):
            pass

    if not REQUIREMENTS.exists():
        log(f"⚠ requirements.txt не знайдено поруч ({REQUIREMENTS}).")
        return

    log("Оновлюю pip…")
    try:
        subprocess.check_call(
            [str(py), "-m", "pip", "install", "--upgrade", "pip", "--disable-pip-version-check"]
        )
    except Exception as e:
        log(f"Попередження: не вдалося оновити pip ({e}), спробуємо продовжити.")

    log("Встановлюю залежності (mediapipe, opencv, pyautogui)… "
        "перший раз це може зайняти 1-3 хвилини.")
    subprocess.check_call(
        [str(py), "-m", "pip", "install", "-r", str(REQUIREMENTS),
         "--disable-pip-version-check"]
    )

    try:
        marker.parent.mkdir(parents=True, exist_ok=True)
        marker.write_text(f"{req_mtime}", encoding="utf-8")
    except OSError:
        pass
    log("Залежності встановлено.")


# ── Exec ────────────────────────────────────────────────────────────────────
def run_script(py: Path, forwarded_args: list[str]) -> int:
    """Re-launch ``script.py`` with the interpreter, inheriting stdio."""
    if not SCRIPT_PY.exists():
        log(f"⚠ script.py не знайдено: {SCRIPT_PY}")
        return 2

    cmd = [str(py), "-u", str(SCRIPT_PY), *forwarded_args]
    log(f"Запускаю: {' '.join(cmd)}")
    env = os.environ.copy()
    # Force UTF-8 stdio so cyrillic log lines round-trip through QProcess.
    env["PYTHONIOENCODING"] = "utf-8"
    env["PYTHONUNBUFFERED"] = "1"
    # Tell pyautogui to fail fast if it cannot reach the display.
    env.setdefault("DISPLAY", env.get("DISPLAY", ":0"))

    # subprocess.run inherits our stdio → C++ QProcess keeps reading lines.
    try:
        return subprocess.run(cmd, env=env, check=False).returncode
    except KeyboardInterrupt:
        return 130


def main() -> int:
    ap = argparse.ArgumentParser(add_help=True)
    ap.add_argument("--venv", default=None,
                    help="Override venv location (default: <APPDATA>/JARVIS/jarvis-vision).")
    ap.add_argument("--skip-install", action="store_true",
                    help="Skip pip install step (assume venv is already populated).")
    # Everything after `--` is forwarded verbatim to script.py.
    args, forwarded = ap.parse_known_args()
    
    # Strip the leading `--` separator(s) if present.  Argparse's parse_known_args
    # stops at the first `--` and includes it in the unparsed list; we don't
    # want to pass that literal `--` down to script.py as it would treat it
    # as the start of positional arguments.
    while forwarded and forwarded[0] == '--':
        forwarded.pop(0)

    root = venv_root(args.venv)
    log(f"Кореневий каталог JARVIS Vision: {root}")
    
    # Check Python compatibility
    is_win = sys.platform.startswith("win")
    is_compatible = (3, 8) <= sys.version_info < (3, 13)
    
    is_portable = False
    if not is_compatible:
        if is_win:
            log(f"Поточна версія Python ({sys.version_info.major}.{sys.version_info.minor}) не сумісна з mediapipe.")
            py = setup_portable_python(root)
            is_portable = True
        else:
            raise RuntimeError(
                f"Несумісна версія Python ({sys.version_info.major}.{sys.version_info.minor}). "
                "Будь ласка, встановіть Python 3.11 або 3.12."
            )
    else:
        py = ensure_venv(root)
        
    if not args.skip_install:
        ensure_deps(py, root, is_portable=is_portable)
    return run_script(py, forwarded)


if __name__ == "__main__":
    sys.exit(main())
