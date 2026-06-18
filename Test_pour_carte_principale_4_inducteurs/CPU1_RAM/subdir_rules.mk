################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
%.obj: ../%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"C:/ti/ccs1260/ccs/tools/compiler/ti-cgt-c2000_22.6.1.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla2 --float_support=fpu32 --tmu_support=tmu0 --vcu_support=vcrc -O3 --opt_for_speed=5 --fp_mode=relaxed --fp_reassoc=on --include_path="C:/Users/CanUl/Documents/TB/Travail de Bachelor/Test_pour_carte_principale_4_inducteurs" --include_path="C:/ti/c2000/C2000Ware_5_02_00_00" --include_path="C:/Users/CanUl/Documents/TB/Travail de Bachelor/Test_pour_carte_principale_4_inducteurs/device" --include_path="C:/ti/c2000/C2000Ware_5_02_00_00/driverlib/f28003x/driverlib" --include_path="C:/ti/c2000/C2000Ware_5_02_00_00/libraries/calibration/hrpwm/f28003x/include" --include_path="C:/ti/ccs1260/ccs/tools/compiler/ti-cgt-c2000_22.6.1.LTS/include" --advice:performance=all --define=DEBUG --define=RAM --diag_suppress=10063 --diag_warning=225 --diag_wrap=off --display_error_number --gen_func_subsections=on --abi=eabi --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" --include_path="C:/Users/CanUl/Documents/TB/Travail de Bachelor/Test_pour_carte_principale_4_inducteurs/CPU1_RAM/syscfg" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

build-2088420289: ../hrpwm_ex1_duty_sfo.syscfg
	@echo 'Building file: "$<"'
	@echo 'Invoking: SysConfig'
	"C:/ti/ccs1260/ccs/utils/sysconfig_1.19.0/sysconfig_cli.bat" --script "C:/Users/CanUl/Documents/TB/Travail de Bachelor/Test_pour_carte_principale_4_inducteurs/hrpwm_ex1_duty_sfo.syscfg" -o "syscfg" -s "C:/ti/c2000/C2000Ware_5_02_00_00/.metadata/sdk.json" -d "F28003x" -p "100PZ" -r "F28003x_100PZ" --compiler ccs
	@echo 'Finished building: "$<"'
	@echo ' '

syscfg/board.c: build-2088420289 ../hrpwm_ex1_duty_sfo.syscfg
syscfg/board.h: build-2088420289
syscfg/board.cmd.genlibs: build-2088420289
syscfg/board.opt: build-2088420289
syscfg/board.json: build-2088420289
syscfg/pinmux.csv: build-2088420289
syscfg/epwm.dot: build-2088420289
syscfg/adc.dot: build-2088420289
syscfg/c2000ware_libraries.cmd.genlibs: build-2088420289
syscfg/c2000ware_libraries.opt: build-2088420289
syscfg/c2000ware_libraries.c: build-2088420289
syscfg/c2000ware_libraries.h: build-2088420289
syscfg/clocktree.h: build-2088420289
syscfg: build-2088420289

syscfg/%.obj: ./syscfg/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"C:/ti/ccs1260/ccs/tools/compiler/ti-cgt-c2000_22.6.1.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla2 --float_support=fpu32 --tmu_support=tmu0 --vcu_support=vcrc -O3 --opt_for_speed=5 --fp_mode=relaxed --fp_reassoc=on --include_path="C:/Users/CanUl/Documents/TB/Travail de Bachelor/Test_pour_carte_principale_4_inducteurs" --include_path="C:/ti/c2000/C2000Ware_5_02_00_00" --include_path="C:/Users/CanUl/Documents/TB/Travail de Bachelor/Test_pour_carte_principale_4_inducteurs/device" --include_path="C:/ti/c2000/C2000Ware_5_02_00_00/driverlib/f28003x/driverlib" --include_path="C:/ti/c2000/C2000Ware_5_02_00_00/libraries/calibration/hrpwm/f28003x/include" --include_path="C:/ti/ccs1260/ccs/tools/compiler/ti-cgt-c2000_22.6.1.LTS/include" --advice:performance=all --define=DEBUG --define=RAM --diag_suppress=10063 --diag_warning=225 --diag_wrap=off --display_error_number --gen_func_subsections=on --abi=eabi --preproc_with_compile --preproc_dependency="syscfg/$(basename $(<F)).d_raw" --include_path="C:/Users/CanUl/Documents/TB/Travail de Bachelor/Test_pour_carte_principale_4_inducteurs/CPU1_RAM/syscfg" --obj_directory="syscfg" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


