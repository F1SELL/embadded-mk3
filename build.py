#!/usr/bin/env python3
"""Build with the ARM compiler bundled in STM32CubeIDE, or ARM_GCC_BIN."""
from pathlib import Path
import os, glob, shutil, subprocess
root=Path(__file__).resolve().parent
os.chdir(root)
compiler=os.environ.get('ARM_GCC_BIN')
if not compiler:
    installed=glob.glob('/Applications/STM32CubeIDE.app/Contents/Eclipse/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools*/tools/bin')
    compiler=installed[0] if installed else str(Path(shutil.which('arm-none-eabi-gcc') or '/missing/arm-none-eabi-gcc').parent)
tool=lambda name: str(Path(compiler)/('arm-none-eabi-'+name))
build=root/'build';build.mkdir(exist_ok=True)
rtos='Middlewares/Third_Party/FreeRTOS/Source'
includes=['Core/Inc','Drivers/STM32F4xx_HAL_Driver/Inc','Drivers/STM32F4xx_HAL_Driver/Inc/Legacy','Drivers/CMSIS/Include','Drivers/CMSIS/Device/ST/STM32F4xx/Include',rtos+'/include',rtos+'/CMSIS_RTOS',rtos+'/portable/GCC/ARM_CM4F']
flags=['-mcpu=cortex-m4','-mthumb','-mfpu=fpv4-sp-d16','-mfloat-abi=hard','-DUSE_HAL_DRIVER','-DSTM32F407xx','-Og','-g3','-ffunction-sections','-fdata-sections','-Wall','-Wextra','-Wno-unused-parameter','-std=gnu11']+['-I'+p for p in includes]
sources=sorted(glob.glob('Core/Src/*.c')+glob.glob('Drivers/STM32F4xx_HAL_Driver/Src/*.c')+glob.glob(rtos+'/*.c')+glob.glob(rtos+'/CMSIS_RTOS/*.c')+[rtos+'/portable/GCC/ARM_CM4F/port.c',rtos+'/portable/MemMang/heap_4.c']+glob.glob('startup/*.s'))
objects=[]
for src in sources:
    obj=build/(src.replace('/','_')+'.o');objects.append(str(obj))
    subprocess.run([tool('gcc'),*flags,'-c',src,'-o',str(obj)],check=True)
elf=build/'RedBall_FreeRTOS.elf'
subprocess.run([tool('gcc'),*flags,*objects,'-TSTM32F407VGTx_FLASH.ld','--specs=nano.specs','--specs=nosys.specs','-Wl,--gc-sections','-Wl,-Map='+str(build/'RedBall_FreeRTOS.map'),'-o',str(elf)],check=True)
for fmt,suffix in [('binary','bin'),('ihex','hex')]:subprocess.run([tool('objcopy'),'-O',fmt,str(elf),str(elf.with_suffix('.'+suffix))],check=True)
subprocess.run([tool('size'),str(elf)],check=True)
