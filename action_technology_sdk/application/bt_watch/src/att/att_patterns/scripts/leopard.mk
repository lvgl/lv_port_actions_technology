ROOT ?= ${CURDIR}

DEFINES := -DCONFIG_CPU_CORTEX_M33 -DCONFIG_SOC_LARK -DCONFIG_FPU -DCONFIG_FP_HARDABI

ifeq ($(TOOLCHAIN), GCC)
ifneq ($(shell which arm-none-eabi-gcc),)
	CROSS_COMPILE ?= arm-none-eabi-
else
	CROSS_COMPILE ?= /opt/gcc-arm-none-eabi-9-2020-q2-update/bin/arm-none-eabi-
endif

CC	:= ${CROSS_COMPILE}gcc
CPP	:= ${CROSS_COMPILE}cpp
LDCC := ${CROSS_COMPILE}gcc
AS	:= ${CROSS_COMPILE}gcc
AR	:= ${CROSS_COMPILE}ar
LD	:= ${CROSS_COMPILE}ld
OC	:= ${CROSS_COMPILE}objcopy
OD	:= ${CROSS_COMPILE}objdump
NM	:= ${CROSS_COMPILE}nm
PP	:= ${CROSS_COMPILE}gcc -E ${CFLAGS}

READELF := $(CROSS_COMPILE)readelf
STRIP   := $(CROSS_COMPILE)strip
OBJSIZE := $(CROSS_COMPILE)size

DEFINES := -DCONFIG_CPU_CORTEX_M33 -DCONFIG_SOC_LARK

ASFLAGS			+= 	-nostdinc -ffreestanding -Wa,--fatal-warnings	\
				-mcpu=cortex-m33+nodsp -mthumb  -Werror -Wmissing-include-dirs	\
				-D__ASSEMBLY__	\
				${DEFINES} ${INCLUDES} -DDEBUG=${DEBUG} -g
CFLAGS			+= 	-nostdinc  -ffreestanding -Wall  -mfloat-abi=hard  \
				 -mcpu=cortex-m33+nodsp -mthumb  -Wmissing-include-dirs	 \
				 -std=gnu99 -c -Os  -mno-unaligned-access  \
				${DEFINES} ${INCLUDES} -DDEBUG=${DEBUG} -g -gdwarf-4 -mabi=aapcs -mfpu=fpv5-sp-d16


CFLAGS			+=	-ffunction-sections -fdata-sections 


LDFLAGS			+=	--fatal-warnings -O1 

TOOLCHAIN_LIB_DIR := $(shell dirname `$(CC) $(CFLAGS) -print-libgcc-file-name`)
LIB_DIR += $(TOOLCHAIN_LIB_DIR)

LIB_INCLUDE := $(foreach n, $(LIB_DIR), -L$(n))
LDFLAGS			+=	--gc-sections $(LIB_INCLUDE)  -Map=$(MAPFILE) --script $(LINKERFILE) -lgcc

ARCMD           =  $$(AR) rcs $$@ $(OBJS)

LDCCFLAGS       += $(ASFLAGS) -x c -P -E 

OCFLAGS         += -O binary $$< $$@
ODFLAGS         += -dx $$< > $$@

else

# keil toolchain path
#CROSS_COMPILE = /c/Keil_v5/ARM/ARMCLANG/bin

CC = armclang
LDCC = armclang
AS = armasm
AR = armar
LD = armlink
OC = fromelf
OD = fromelf
DEFINES         += -D__UVISION_VERSION="531" -D__MICROLIB -DARMCM4_FP -DCONFIG_FPU -DCONFIG_FP_HARDABI

ASFLAGS			+= --target=arm-arm-none-eabi -mcpu=cortex-m33+nodsp -mfpu=fpv5-sp-d16 -mfloat-abi=hard -masm=auto \
					-mlittle-endian -gdwarf-3 -Wa,armasm,--pd -D__ASSEMBLY__ \
				${DEFINES} ${INCLUDES}

CFLAGS			+= -c -std=c99   -mcpu=cortex-m33+nodsp -mfpu=fpv5-sp-d16 -mfloat-abi=hard  -mthumb \
				-mabi=aapcs -Wall -Wformat -Wformat-security -Wno-format-zero-length -Wno-main \
				-Wno-pointer-sign -Wpointer-arith -Wno-address-of-packed-member -Werror=implicit-int \
				-ffreestanding -fno-common -fno-asynchronous-unwind-tables -fno-pie -fno-pic -fno-strict-overflow -fno-rtti\
				-funsigned-char -fshort-enums -fshort-wchar -ffunction-sections -fdata-sections -MD \
				${DEFINES} ${INCLUDES}  -xc --target=arm-arm-none-eabi -mlittle-endian -gdwarf-3 -Oz 

#LDFLAGS			+= --cpu=Cortex-M4.fp.sp --remove --diag_suppress 6092,6124,6306,6314,6320,6329,6312 --strict
LDFLAGS			+= --remove --diag_suppress 6092,6124,6306,6314,6320,6329,6312 --strict

LDFLAGS         += --no_startup --entry pattern_main

ARCMD           =  $$(AR)  $$@ -r $(OBJS)

LDFLAGS			+=	$(LIB_INCLUDE) --library_type=microlib  --summary_stderr --info summarysizes --map --load_addr_map_info --xref --callgraph --symbols \
					--info sizes --info totals --info unused --info veneers --list=$(MAPFILE) --scatter $(LINKERFILE)

LDCCFLAGS       += $(ASFLAGS) -x c -P -E 

OCFLAGS         += --bin --output=$$@ $$<
ODFLAGS         += --text -a -c --output=$$@ $$<

endif

INCLUDES	+=




# Checkpatch rules
CKPATCH := ${ROOT}/scripts/codestyle/checkpatch.pl
CKFLAGS := --mailback --no-tree -f --emacs --summary-file --show-types
CKFLAGS += --ignore BRACES,PRINTK_WITHOUT_KERN_LEVEL,SPLIT_STRING,CONST_STRUCT
CKFLAGS += --ignore NEW_TYPEDEFS,VOLATILE,DATE_TIME,ARRAY_SIZE,PREFER_KERNEL_TYPES,PREFER_PACKED
CKFLAGS += --max-line-length=100
CKFLAGS += --exclude=ddr

#-pedantic

ifeq (${V},0)
	Q=@
else
	Q=
endif

# Default build string (git branch and commit)
ifeq (${BUILD_STRING},)
	BUILD_STRING	:=	$(shell git log -n 1 --pretty=format:"%h")
endif

all: msg_start

msg_start:
	@echo "Building ${PLAT}"

clean:
	@echo "  CLEAN"
	@-rm -f *.o *.map *.lst *.elf *.$(OUTBINSUFFIX) *.ld *.sct *.htm  *.a $(OBJS) $(PREREQUISITES) $(LINKERFILE) $(C_DEPS)

define MAKE_C

$(eval OBJ := $(1)/$(patsubst %.c,%.o,$(2)))
$(eval PREREQUISITES := $(patsubst %.o,%.d,$(OBJ)))
$(eval CHK_C := $(1)/$(patsubst %.c,%.c.chk,$(2)))

$(OBJ) : $(2)
	@echo "  CC      $$<"
	$$(Q)$$(CC) $$(CFLAGS) -c $$< -o $$@

$(PREREQUISITES) : $(2)
	@echo "  DEPS    $$@"
	@mkdir -p $(1)
	$$(Q)$$(CC) $$(CFLAGS) -M -MT $(OBJ) -MF $$@ $$<

ifdef IS_ANYTHING_TO_BUILD
-include $(PREREQUISITES)
endif

$(CHK_C) : $(2)
	@echo "  CHK     $$<"
	$$(Q)$$(CKPATCH) $$(CKFLAGS) $$<

check : $(CHK_C)

endef


define MAKE_H

$(eval CHK_H := $(patsubst %.h,%.h.chk,$(1)))

$(CHK_H) : $(1)
	@echo "  CHK     $$<"
	$$(Q)$$(CKPATCH) $$(CKFLAGS) $$<

check : $(CHK_H)

endef


define MAKE_S

$(eval OBJ := $(1)/$(patsubst %.S,%.o,$(2)))
$(eval PREREQUISITES := $(patsubst %.o,%.d,$(OBJ)))

$(OBJ) : $(2)
	@echo "  AS      $$<"
	$$(Q)$$(AS) $$(ASFLAGS) -c $$< -o $$@

$(PREREQUISITES) : $(2)
	@echo "  DEPS    $$@"
	@mkdir -p $(1)
	$$(Q)$$(AS) $$(ASFLAGS) -M -MT $(OBJ) -MF $$@ $$<

ifdef IS_ANYTHING_TO_BUILD
-include $(PREREQUISITES)
endif

endef


define MAKE_LD

$(eval PREREQUISITES := $(1).d)

$(1) : $(2)
	@echo "  PP      $$<"
	$$(Q)$$(LDCC) $$(LDCCFLAGS) -o $$@ $$<

$(PREREQUISITES) : $(2)
	@echo "  DEPS    $$@"
	mkdir -p $$(dir $$@)
	$$(Q)$$(AS) $$(ASFLAGS) -M -MT $(1) -MF $$@ $$<

ifdef IS_ANYTHING_TO_BUILD
-include $(PREREQUISITES)
endif

endef


define MAKE_OBJS
	$(eval C_OBJS := $(filter %.c,$(2)))
	$(eval C_DEPS := $(patsubst %.c,%.d,$(2)))
	$(eval REMAIN := $(filter-out %.c,$(2)))
	$(eval $(foreach obj,$(C_OBJS),$(call MAKE_C,$(1),$(obj))))

	$(eval S_OBJS := $(filter %.S,$(REMAIN)))
	$(eval REMAIN := $(filter-out %.S,$(REMAIN)))
	$(eval $(foreach obj,$(S_OBJS),$(call MAKE_S,$(1),$(obj))))

	$(and $(REMAIN),$(error Unexpected source files present: $(REMAIN)))

	$(eval H_DIRS := $(subst -I,,$(subst $(CURDIR),.,$(INCLUDES))))
	$(eval H_FILES := $(foreach d,$(H_DIRS),$(wildcard $(d)/*.h)))
	$(eval $(foreach h,$(H_FILES),$(call MAKE_H,$(h))))
endef

# NOTE: The line continuation '\' is required in the next define otherwise we
# end up with a line-feed characer at the end of the last c filename.
# Also bare this issue in mind if extending the list of supported filetypes.
define SOURCES_TO_OBJS
	$(patsubst %.c,%.o,$(filter %.c,$(1))) \
	$(patsubst %.S,%.o,$(filter %.S,$(1)))
endef

#	$(eval CFLAGS     += -pie)	

define MAKE_LIB
	$(eval BUILD_DIR  := .)
	$(eval SOURCES    := $(2))
	$(eval OBJS       := $(addprefix $(BUILD_DIR)/,$(call SOURCES_TO_OBJS,$(SOURCES))))
	$(eval LIB        := $(1))
	$(eval $(call MAKE_OBJS,$(BUILD_DIR),$(SOURCES)))

$(BUILD_DIR) :
	$$(Q)mkdir -p "$$@"

$(LIB) : $(OBJS)
	@echo "  AR      $$@"
	$$(Q) $(ARCMD)
	@echo "Built $$@ successfully"
	@echo

.PHONY : $(LIB)

all :  $(BUILD_DIR) $(LIB)

endef

define MAKE_PROG
	$(eval BUILD_DIR  := .)
	$(eval SOURCES    := $(2))
	$(eval OBJS       := $(addprefix $(BUILD_DIR)/,$(call SOURCES_TO_OBJS,$(SOURCES))))
	$(eval LINKER_SFILE := $(filter %.S,$(3)))
	$(eval LINKERFILE := $(basename $(LINKER_SFILE)))
	$(eval MAPFILE    := $(1).map)
	$(eval ELF        := $(1).elf)
	$(eval DUMP       := $(1).lst)
	$(eval BIN        := $(1).$(OUTBINSUFFIX))

	$(eval $(call MAKE_OBJS,$(BUILD_DIR),$(SOURCES)))

	$(if $(LINKER_SFILE), 							\
		$(eval $(call MAKE_LD,$(LINKERFILE),$(LINKER_SFILE))),		\
		$(eval LINKERFILE = $(3)))

$(BUILD_DIR) :
	$$(Q)mkdir -p "$$@"

$(ELF) : $(OBJS) $(LINKERFILE)
	@echo "  LD      $$@"

#	@echo 'const char build_string[] = "${BUILD_STRING}";' | \
#		$$(CC) $$(CFLAGS) -xc - -o $(BUILD_DIR)/build_message.o

#	$$(Q)$$(LD) -o $$@ $$(LDFLAGS)   \
#					$(BUILD_DIR)/build_message.o $(OBJS) $(LIBS) 

	$$(Q)$$(LD) -o $$@ $$(LDFLAGS)   \
					 $(OBJS) $(LIBS) 
$(DUMP) : $(ELF)
	@echo "  OD      $$@"
	$${Q}$${OD} $(ODFLAGS) 

$(BIN) : $(ELF)
	@echo "  BIN     $$@"
	$$(Q)$$(OC) $(OCFLAGS)  
	@echo
	@echo "Built $$@ successfully"
	@echo

.PHONY : prog_$(1)
prog_$(1) : $(BUILD_DIR) $(BIN) $(DUMP)

all : prog_$(1)

endef
