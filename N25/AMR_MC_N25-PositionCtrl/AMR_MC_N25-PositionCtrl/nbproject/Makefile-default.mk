#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Include project Makefile
ifeq "${IGNORE_LOCAL}" "TRUE"
# do not include local makefile. User is passing all local related variables already
else
include Makefile
# Include makefile containing local settings
ifeq "$(wildcard nbproject/Makefile-local-default.mk)" "nbproject/Makefile-local-default.mk"
include nbproject/Makefile-local-default.mk
endif
endif

# Environment
MKDIR=gnumkdir -p
RM=rm -f 
MV=mv 
CP=cp 

# Macros
CND_CONF=default
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
IMAGE_TYPE=debug
OUTPUT_SUFFIX=elf
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=dist/${CND_CONF}/${IMAGE_TYPE}/AMR_MC_N25.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
else
IMAGE_TYPE=production
OUTPUT_SUFFIX=hex
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=dist/${CND_CONF}/${IMAGE_TYPE}/AMR_MC_N25.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
endif

ifeq ($(COMPARE_BUILD), true)
COMPARISON_BUILD=-mafrlcsj
else
COMPARISON_BUILD=
endif

ifdef SUB_IMAGE_ADDRESS

else
SUB_IMAGE_ADDRESS_COMMAND=
endif

# Object Directory
OBJECTDIR=build/${CND_CONF}/${IMAGE_TYPE}

# Distribution Directory
DISTDIR=dist/${CND_CONF}/${IMAGE_TYPE}

# Source Files Quoted if spaced
SOURCEFILES_QUOTED_IF_SPACED=can.c adc.c qei.c main.c pwm.c spi.c timer.c status.c execution.c control.c init.c FOC.c home.c SV_table.c port.c

# Object Files Quoted if spaced
OBJECTFILES_QUOTED_IF_SPACED=${OBJECTDIR}/can.o ${OBJECTDIR}/adc.o ${OBJECTDIR}/qei.o ${OBJECTDIR}/main.o ${OBJECTDIR}/pwm.o ${OBJECTDIR}/spi.o ${OBJECTDIR}/timer.o ${OBJECTDIR}/status.o ${OBJECTDIR}/execution.o ${OBJECTDIR}/control.o ${OBJECTDIR}/init.o ${OBJECTDIR}/FOC.o ${OBJECTDIR}/home.o ${OBJECTDIR}/SV_table.o ${OBJECTDIR}/port.o
POSSIBLE_DEPFILES=${OBJECTDIR}/can.o.d ${OBJECTDIR}/adc.o.d ${OBJECTDIR}/qei.o.d ${OBJECTDIR}/main.o.d ${OBJECTDIR}/pwm.o.d ${OBJECTDIR}/spi.o.d ${OBJECTDIR}/timer.o.d ${OBJECTDIR}/status.o.d ${OBJECTDIR}/execution.o.d ${OBJECTDIR}/control.o.d ${OBJECTDIR}/init.o.d ${OBJECTDIR}/FOC.o.d ${OBJECTDIR}/home.o.d ${OBJECTDIR}/SV_table.o.d ${OBJECTDIR}/port.o.d

# Object Files
OBJECTFILES=${OBJECTDIR}/can.o ${OBJECTDIR}/adc.o ${OBJECTDIR}/qei.o ${OBJECTDIR}/main.o ${OBJECTDIR}/pwm.o ${OBJECTDIR}/spi.o ${OBJECTDIR}/timer.o ${OBJECTDIR}/status.o ${OBJECTDIR}/execution.o ${OBJECTDIR}/control.o ${OBJECTDIR}/init.o ${OBJECTDIR}/FOC.o ${OBJECTDIR}/home.o ${OBJECTDIR}/SV_table.o ${OBJECTDIR}/port.o

# Source Files
SOURCEFILES=can.c adc.c qei.c main.c pwm.c spi.c timer.c status.c execution.c control.c init.c FOC.c home.c SV_table.c port.c



CFLAGS=
ASFLAGS=
LDLIBSOPTIONS=

############# Tool locations ##########################################
# If you copy a project from one host to another, the path where the  #
# compiler is installed may be different.                             #
# If you open this project with MPLAB X in the new host, this         #
# makefile will be regenerated and the paths will be corrected.       #
#######################################################################
# fixDeps replaces a bunch of sed/cat/printf statements that slow down the build
FIXDEPS=fixDeps

.build-conf:  ${BUILD_SUBPROJECTS}
ifneq ($(INFORMATION_MESSAGE), )
	@echo $(INFORMATION_MESSAGE)
endif
	${MAKE}  -f nbproject/Makefile-default.mk dist/${CND_CONF}/${IMAGE_TYPE}/AMR_MC_N25.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}

MP_PROCESSOR_OPTION=32MK1024MCF064
MP_LINKER_FILE_OPTION=
# ------------------------------------------------------------------------------------
# Rules for buildStep: assemble
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: assembleWithPreprocess
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: compile
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${OBJECTDIR}/can.o: can.c  .generated_files/flags/default/47ff7ed46f6c9349f010c67a1c64f049a3d61e77 .generated_files/flags/default/927d2e1b0399ec7b52e9fd6164ad1d90ae9d9944
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/can.o.d 
	@${RM} ${OBJECTDIR}/can.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/can.o.d" -o ${OBJECTDIR}/can.o can.c    -DXPRJ_default=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)    
	
${OBJECTDIR}/adc.o: adc.c  .generated_files/flags/default/6b5f4bfa3a65513e7082611c3c09d84141825bc5 .generated_files/flags/default/927d2e1b0399ec7b52e9fd6164ad1d90ae9d9944
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/adc.o.d 
	@${RM} ${OBJECTDIR}/adc.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/adc.o.d" -o ${OBJECTDIR}/adc.o adc.c    -DXPRJ_default=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)    
	
${OBJECTDIR}/qei.o: qei.c  .generated_files/flags/default/67b5c03af3972d22645c874c069ab182ddbf95c8 .generated_files/flags/default/927d2e1b0399ec7b52e9fd6164ad1d90ae9d9944
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/qei.o.d 
	@${RM} ${OBJECTDIR}/qei.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/qei.o.d" -o ${OBJECTDIR}/qei.o qei.c    -DXPRJ_default=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)    
	
${OBJECTDIR}/main.o: main.c  .generated_files/flags/default/164c001fc3bbb8c749b5ffab3c7eb506f05d81d2 .generated_files/flags/default/927d2e1b0399ec7b52e9fd6164ad1d90ae9d9944
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/main.o.d 
	@${RM} ${OBJECTDIR}/main.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/main.o.d" -o ${OBJECTDIR}/main.o main.c    -DXPRJ_default=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)    
	
${OBJECTDIR}/pwm.o: pwm.c  .generated_files/flags/default/5491624c075742d27b116269d8ab8fa26d1bb84b .generated_files/flags/default/927d2e1b0399ec7b52e9fd6164ad1d90ae9d9944
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/pwm.o.d 
	@${RM} ${OBJECTDIR}/pwm.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/pwm.o.d" -o ${OBJECTDIR}/pwm.o pwm.c    -DXPRJ_default=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)    
	
${OBJECTDIR}/spi.o: spi.c  .generated_files/flags/default/6ecf1f6e9d686f2b9586c908b65a9bf0ba8dc85d .generated_files/flags/default/927d2e1b0399ec7b52e9fd6164ad1d90ae9d9944
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/spi.o.d 
	@${RM} ${OBJECTDIR}/spi.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/spi.o.d" -o ${OBJECTDIR}/spi.o spi.c    -DXPRJ_default=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)    
	
${OBJECTDIR}/timer.o: timer.c  .generated_files/flags/default/576225f06139622ba2af27473118a1a06a02e034 .generated_files/flags/default/927d2e1b0399ec7b52e9fd6164ad1d90ae9d9944
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/timer.o.d 
	@${RM} ${OBJECTDIR}/timer.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/timer.o.d" -o ${OBJECTDIR}/timer.o timer.c    -DXPRJ_default=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)    
	
${OBJECTDIR}/status.o: status.c  .generated_files/flags/default/3aa540151fb3c502577b7db303d85466fe2efe07 .generated_files/flags/default/927d2e1b0399ec7b52e9fd6164ad1d90ae9d9944
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/status.o.d 
	@${RM} ${OBJECTDIR}/status.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/status.o.d" -o ${OBJECTDIR}/status.o status.c    -DXPRJ_default=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)    
	
${OBJECTDIR}/execution.o: execution.c  .generated_files/flags/default/79d156bfde70e7d9a39e64d353bec95a104d48dd .generated_files/flags/default/927d2e1b0399ec7b52e9fd6164ad1d90ae9d9944
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/execution.o.d 
	@${RM} ${OBJECTDIR}/execution.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/execution.o.d" -o ${OBJECTDIR}/execution.o execution.c    -DXPRJ_default=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)    
	
${OBJECTDIR}/control.o: control.c  .generated_files/flags/default/9b28bd409b2f9dd4af4cd61b01c2fd355a761626 .generated_files/flags/default/927d2e1b0399ec7b52e9fd6164ad1d90ae9d9944
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/control.o.d 
	@${RM} ${OBJECTDIR}/control.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/control.o.d" -o ${OBJECTDIR}/control.o control.c    -DXPRJ_default=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)    
	
${OBJECTDIR}/init.o: init.c  .generated_files/flags/default/fc3244f65078560f5c18eb6a5b05a862a5ad5a30 .generated_files/flags/default/927d2e1b0399ec7b52e9fd6164ad1d90ae9d9944
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/init.o.d 
	@${RM} ${OBJECTDIR}/init.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/init.o.d" -o ${OBJECTDIR}/init.o init.c    -DXPRJ_default=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)    
	
${OBJECTDIR}/FOC.o: FOC.c  .generated_files/flags/default/c51b6dc2bb221566af4c9ce7671158b92dee5ecd .generated_files/flags/default/927d2e1b0399ec7b52e9fd6164ad1d90ae9d9944
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/FOC.o.d 
	@${RM} ${OBJECTDIR}/FOC.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/FOC.o.d" -o ${OBJECTDIR}/FOC.o FOC.c    -DXPRJ_default=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)    
	
${OBJECTDIR}/home.o: home.c  .generated_files/flags/default/1f87ff50fdcd1f81efc2f452ddf8adac28d3a372 .generated_files/flags/default/927d2e1b0399ec7b52e9fd6164ad1d90ae9d9944
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/home.o.d 
	@${RM} ${OBJECTDIR}/home.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/home.o.d" -o ${OBJECTDIR}/home.o home.c    -DXPRJ_default=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)    
	
${OBJECTDIR}/SV_table.o: SV_table.c  .generated_files/flags/default/e52964193bdc503a7ba999b75a930363bd6a0808 .generated_files/flags/default/927d2e1b0399ec7b52e9fd6164ad1d90ae9d9944
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/SV_table.o.d 
	@${RM} ${OBJECTDIR}/SV_table.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/SV_table.o.d" -o ${OBJECTDIR}/SV_table.o SV_table.c    -DXPRJ_default=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)    
	
${OBJECTDIR}/port.o: port.c  .generated_files/flags/default/6495090dab079788b0df1c70d706be51b4457453 .generated_files/flags/default/927d2e1b0399ec7b52e9fd6164ad1d90ae9d9944
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/port.o.d 
	@${RM} ${OBJECTDIR}/port.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/port.o.d" -o ${OBJECTDIR}/port.o port.c    -DXPRJ_default=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)    
	
else
${OBJECTDIR}/can.o: can.c  .generated_files/flags/default/77828299bcd4bcb2b2323fa18888b79348a3df09 .generated_files/flags/default/927d2e1b0399ec7b52e9fd6164ad1d90ae9d9944
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/can.o.d 
	@${RM} ${OBJECTDIR}/can.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/can.o.d" -o ${OBJECTDIR}/can.o can.c    -DXPRJ_default=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)    
	
${OBJECTDIR}/adc.o: adc.c  .generated_files/flags/default/80b1a9019e9807f87d395d7005c20bbc05e1517d .generated_files/flags/default/927d2e1b0399ec7b52e9fd6164ad1d90ae9d9944
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/adc.o.d 
	@${RM} ${OBJECTDIR}/adc.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/adc.o.d" -o ${OBJECTDIR}/adc.o adc.c    -DXPRJ_default=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)    
	
${OBJECTDIR}/qei.o: qei.c  .generated_files/flags/default/f2b430cdf8068b3b8aefcbba385fce1a4b2b0e29 .generated_files/flags/default/927d2e1b0399ec7b52e9fd6164ad1d90ae9d9944
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/qei.o.d 
	@${RM} ${OBJECTDIR}/qei.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/qei.o.d" -o ${OBJECTDIR}/qei.o qei.c    -DXPRJ_default=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)    
	
${OBJECTDIR}/main.o: main.c  .generated_files/flags/default/969afd9c13e137ba2eba43e106775c841a8cf23b .generated_files/flags/default/927d2e1b0399ec7b52e9fd6164ad1d90ae9d9944
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/main.o.d 
	@${RM} ${OBJECTDIR}/main.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/main.o.d" -o ${OBJECTDIR}/main.o main.c    -DXPRJ_default=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)    
	
${OBJECTDIR}/pwm.o: pwm.c  .generated_files/flags/default/3131e761c5297a5262a23af560a1497f14255afb .generated_files/flags/default/927d2e1b0399ec7b52e9fd6164ad1d90ae9d9944
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/pwm.o.d 
	@${RM} ${OBJECTDIR}/pwm.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/pwm.o.d" -o ${OBJECTDIR}/pwm.o pwm.c    -DXPRJ_default=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)    
	
${OBJECTDIR}/spi.o: spi.c  .generated_files/flags/default/45fc5b452ad5108c49983c639bbedab09f07ec6e .generated_files/flags/default/927d2e1b0399ec7b52e9fd6164ad1d90ae9d9944
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/spi.o.d 
	@${RM} ${OBJECTDIR}/spi.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/spi.o.d" -o ${OBJECTDIR}/spi.o spi.c    -DXPRJ_default=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)    
	
${OBJECTDIR}/timer.o: timer.c  .generated_files/flags/default/697eff70321ee0e03ababa23a37535fa925cf007 .generated_files/flags/default/927d2e1b0399ec7b52e9fd6164ad1d90ae9d9944
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/timer.o.d 
	@${RM} ${OBJECTDIR}/timer.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/timer.o.d" -o ${OBJECTDIR}/timer.o timer.c    -DXPRJ_default=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)    
	
${OBJECTDIR}/status.o: status.c  .generated_files/flags/default/6622dfeebd59596d70a3a29ad16e0da03eaac692 .generated_files/flags/default/927d2e1b0399ec7b52e9fd6164ad1d90ae9d9944
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/status.o.d 
	@${RM} ${OBJECTDIR}/status.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/status.o.d" -o ${OBJECTDIR}/status.o status.c    -DXPRJ_default=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)    
	
${OBJECTDIR}/execution.o: execution.c  .generated_files/flags/default/f797156289bedf232323dd31cf147227e817e456 .generated_files/flags/default/927d2e1b0399ec7b52e9fd6164ad1d90ae9d9944
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/execution.o.d 
	@${RM} ${OBJECTDIR}/execution.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/execution.o.d" -o ${OBJECTDIR}/execution.o execution.c    -DXPRJ_default=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)    
	
${OBJECTDIR}/control.o: control.c  .generated_files/flags/default/de928df91e9c702dfd43ca23e36430b520c1c82 .generated_files/flags/default/927d2e1b0399ec7b52e9fd6164ad1d90ae9d9944
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/control.o.d 
	@${RM} ${OBJECTDIR}/control.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/control.o.d" -o ${OBJECTDIR}/control.o control.c    -DXPRJ_default=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)    
	
${OBJECTDIR}/init.o: init.c  .generated_files/flags/default/5f844d8f881708b0c86e22ef5e99b82ebe65c49b .generated_files/flags/default/927d2e1b0399ec7b52e9fd6164ad1d90ae9d9944
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/init.o.d 
	@${RM} ${OBJECTDIR}/init.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/init.o.d" -o ${OBJECTDIR}/init.o init.c    -DXPRJ_default=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)    
	
${OBJECTDIR}/FOC.o: FOC.c  .generated_files/flags/default/d81b50f023eadc821dbd5c2f59f8873b037568d1 .generated_files/flags/default/927d2e1b0399ec7b52e9fd6164ad1d90ae9d9944
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/FOC.o.d 
	@${RM} ${OBJECTDIR}/FOC.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/FOC.o.d" -o ${OBJECTDIR}/FOC.o FOC.c    -DXPRJ_default=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)    
	
${OBJECTDIR}/home.o: home.c  .generated_files/flags/default/7d4f3ffe59fd033d61dd8f1ade9e77fcb36d92b9 .generated_files/flags/default/927d2e1b0399ec7b52e9fd6164ad1d90ae9d9944
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/home.o.d 
	@${RM} ${OBJECTDIR}/home.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/home.o.d" -o ${OBJECTDIR}/home.o home.c    -DXPRJ_default=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)    
	
${OBJECTDIR}/SV_table.o: SV_table.c  .generated_files/flags/default/508f54989d4a9a852a6876c34fffe292bf8d7d47 .generated_files/flags/default/927d2e1b0399ec7b52e9fd6164ad1d90ae9d9944
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/SV_table.o.d 
	@${RM} ${OBJECTDIR}/SV_table.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/SV_table.o.d" -o ${OBJECTDIR}/SV_table.o SV_table.c    -DXPRJ_default=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)    
	
${OBJECTDIR}/port.o: port.c  .generated_files/flags/default/283930fd7cba0a8b26a930cdcf53dbff343bf489 .generated_files/flags/default/927d2e1b0399ec7b52e9fd6164ad1d90ae9d9944
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/port.o.d 
	@${RM} ${OBJECTDIR}/port.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/port.o.d" -o ${OBJECTDIR}/port.o port.c    -DXPRJ_default=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)    
	
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: compileCPP
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: link
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
dist/${CND_CONF}/${IMAGE_TYPE}/AMR_MC_N25.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk    
	@${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_CC} $(MP_EXTRA_LD_PRE) -g   -mprocessor=$(MP_PROCESSOR_OPTION)  -o dist/${CND_CONF}/${IMAGE_TYPE}/AMR_MC_N25.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX} ${OBJECTFILES_QUOTED_IF_SPACED}          -DXPRJ_default=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)   -mreserve=data@0x0:0x36F   -Wl,--defsym=__MPLAB_BUILD=1$(MP_EXTRA_LD_POST)$(MP_LINKER_FILE_OPTION),--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,-D=__DEBUG_D,--no-code-in-dinit,--no-dinit-in-serial-mem,-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map",--memorysummary,dist/${CND_CONF}/${IMAGE_TYPE}/memoryfile.xml 
	
else
dist/${CND_CONF}/${IMAGE_TYPE}/AMR_MC_N25.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk   
	@${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_CC} $(MP_EXTRA_LD_PRE)  -mprocessor=$(MP_PROCESSOR_OPTION)  -o dist/${CND_CONF}/${IMAGE_TYPE}/AMR_MC_N25.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX} ${OBJECTFILES_QUOTED_IF_SPACED}          -DXPRJ_default=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -Wl,--defsym=__MPLAB_BUILD=1$(MP_EXTRA_LD_POST)$(MP_LINKER_FILE_OPTION),--no-code-in-dinit,--no-dinit-in-serial-mem,-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map",--memorysummary,dist/${CND_CONF}/${IMAGE_TYPE}/memoryfile.xml 
	${MP_CC_DIR}\\xc32-bin2hex dist/${CND_CONF}/${IMAGE_TYPE}/AMR_MC_N25.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX} 
endif


# Subprojects
.build-subprojects:


# Subprojects
.clean-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r build/default
	${RM} -r dist/default

# Enable dependency checking
.dep.inc: .depcheck-impl

DEPFILES=$(shell mplabwildcard ${POSSIBLE_DEPFILES})
ifneq (${DEPFILES},)
include ${DEPFILES}
endif
