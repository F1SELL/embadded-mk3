#!/usr/bin/env python3
"""Explicitly run this script to program the SDK-1.1M connected to USB DBG."""
from pathlib import Path
import glob,os,subprocess
root=Path(__file__).resolve().parent
paths=glob.glob('/Applications/STM32CubeIDE.app/Contents/Eclipse/plugins/com.st.stm32cube.ide.mcu.externaltools.openocd.macos64_*/tools')
tools=Path(os.environ.get('OPENOCD_TOOLS',paths[0] if paths else '/missing'))
script_paths=glob.glob('/Applications/STM32CubeIDE.app/Contents/Eclipse/plugins/com.st.stm32cube.ide.mcu.debug.openocd_*/resources/openocd/st_scripts')
scripts=Path(os.environ.get('OPENOCD_SCRIPTS',script_paths[0] if script_paths else str(tools/'share/openocd/scripts')))
elf=root/'build/RedBall_FreeRTOS.elf'
if not elf.exists():raise SystemExit('First run: python3 build.py')
subprocess.run([str(tools/'bin/openocd'),'-s',str(scripts),'-f',str(root/'SDK11M.cfg'),'-c',f'program {{{elf}}} verify reset exit'],check=True)
