# 2026-11-23T15:44:02.316465600
import vitis

client = vitis.create_client()
client.set_workspace(path="vitis_ws")

platform = client.create_platform_component(name = "pynq_platform",hw_design = "$COMPONENT_LOCATION/../../hw/system.xsa",os = "standalone",cpu = "ps7_cortexa9_0",domain_name = "standalone_ps7_cortexa9_0",compiler = "gcc")

comp = client.create_app_component(name="fft_dma_test",platform = "$COMPONENT_LOCATION/../pynq_platform/export/pynq_platform/pynq_platform.xpfm",domain = "standalone_ps7_cortexa9_0",template = "empty_application")

component = client.get_component(name="fft_dma_test")

lscript = component.get_ld_script(path="D:\radar\vitis_ws\fft_dma_test\src\lscript.ld")

lscript.set_stack_size("0x100000")

lscript.set_heap_size("0x100000")

platform = client.get_component(name="pynq_platform")
status = platform.build()

comp = client.get_component(name="fft_dma_test")
comp.build()

