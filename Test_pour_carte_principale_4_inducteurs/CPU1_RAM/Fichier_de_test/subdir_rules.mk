################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
Fichier_de_test/%.obj: ../Fichier_de_test/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"/home/can/ti/ccs1260/ccs/tools/compiler/ti-cgt-c2000_22.6.1.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla2 --float_support=fpu32 --tmu_support=tmu0 --vcu_support=vcrc -O3 --opt_for_speed=5 --fp_mode=relaxed --fp_reassoc=on --include_path="/home/can/TB/Travail-de-Bachelor/Test_pour_carte_principale_4_inducteurs" --include_path="/home/can/ti/C2000Ware_5_02_00_00" --include_path="/home/can/TB/Travail-de-Bachelor/Test_pour_carte_principale_4_inducteurs/device" --include_path="/home/can/ti/C2000Ware_5_02_00_00/driverlib/f28003x/driverlib" --include_path="/home/can/ti/C2000Ware_5_02_00_00/libraries/calibration/hrpwm/f28003x/include" --include_path="/home/can/ti/ccs1260/ccs/tools/compiler/ti-cgt-c2000_22.6.1.LTS/include" --advice:performance=all --define=DEBUG --define=RAM --diag_suppress=10063 --diag_warning=225 --diag_wrap=off --display_error_number --gen_func_subsections=on --abi=eabi --preproc_with_compile --preproc_dependency="Fichier_de_test/$(basename $(<F)).d_raw" --include_path="/home/can/TB/Travail-de-Bachelor/Test_pour_carte_principale_4_inducteurs/CPU1_RAM/syscfg" --obj_directory="Fichier_de_test" $(GEN_OPTS__FLAG) "$(shell echo $<)"
	@echo 'Finished building: "$<"'
	@echo ' '


