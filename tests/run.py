#!/usr/bin/env python3
"""Build and test game.c on macOS. No board is needed."""
from pathlib import Path
import subprocess,sys
root=Path(__file__).resolve().parents[1]
sdk=Path('/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk')
if not sdk.exists():sdk=Path(subprocess.check_output(['xcrun','--show-sdk-path'],text=True).strip())
subprocess.run(['clang','-isysroot',str(sdk),'-shared','-fPIC','-std=c11','-Wall','-Wextra','-Werror','-I'+str(root/'Core/Inc'),str(root/'Core/Src/game.c'),'-o',str(root/'tests/game.dylib')],check=True)
subprocess.run([sys.executable,str(root/'tests/test_game.py')],check=True)
