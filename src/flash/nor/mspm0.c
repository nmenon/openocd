// SPDX-License-Identifier: GPL-2.0-or-later

/***************************************************************************
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "jtag/interface.h"
#include "imp.h"
#include <target/algorithm.h>
#include <target/arm_adi_v5.h>
#include <target/armv7m.h>
#include <helper/bits.h>

/* MPM0 FACTORYREGION registers */
#define FACTORYREGION			0x41c40000
#define TRACEID					(FACTORYREGION + 0x000)
#define DID						(FACTORYREGION + 0x004)
#define USERID					(FACTORYREGION + 0x008)
#define SRAMFLASH				(FACTORYREGION + 0x018)

#define FLASH_BASE_NONMAIN		(0x41c00000)
#define FLASH_BASE_MAIN			(0x0)
#define FLASH_BASE_DATA			(0x41d00000)

/* MPM0 FLASHCTL registers */
#define FLASH_CONTROL_BASE		0x400cd000
#define FCTL_REG_IIDX			(FLASH_CONTROL_BASE + 0x1020)
#define FCTL_REG_IMASK			(FLASH_CONTROL_BASE + 0x1028)
#define FCTL_REG_RIS			(FLASH_CONTROL_BASE + 0x1030)
#define FCTL_REG_MIS			(FLASH_CONTROL_BASE + 0x1038)
#define FCTL_REG_ISET			(FLASH_CONTROL_BASE + 0x1040)
#define FCTL_REG_ICLR			(FLASH_CONTROL_BASE + 0x1048)
#define FCTL_REG_CMDEXEC		(FLASH_CONTROL_BASE + 0x1100)
#define FCTL_REG_CMDTYPE		(FLASH_CONTROL_BASE + 0x1104)
#define FCTL_REG_CMDCTL			(FLASH_CONTROL_BASE + 0x1108)
#define FCTL_REG_CMDADDR		(FLASH_CONTROL_BASE + 0x1120)
#define FCTL_REG_CMDBYTEN		(FLASH_CONTROL_BASE + 0x1124)
#define FCTL_REG_CMDDATAINDEX	(FLASH_CONTROL_BASE + 0x112C)
#define FCTL_REG_CMDDATA0		(FLASH_CONTROL_BASE + 0x1130)
#define FCTL_REG_CMDDATA1		(FLASH_CONTROL_BASE + 0x1134)
#define FCTL_REG_CMDDATA2		(FLASH_CONTROL_BASE + 0x1138)
#define FCTL_REG_CMDDATA3		(FLASH_CONTROL_BASE + 0x113C)
#define FCTL_REG_CMDDATA4		(FLASH_CONTROL_BASE + 0x1140)
#define FCTL_REG_CMDDATA5		(FLASH_CONTROL_BASE + 0x1144)
#define FCTL_REG_CMDDATA6		(FLASH_CONTROL_BASE + 0x1148)
#define FCTL_REG_CMDDATA7		(FLASH_CONTROL_BASE + 0x114C)
#define FCTL_REG_CMDDATA8		(FLASH_CONTROL_BASE + 0x1150)
#define FCTL_REG_CMDDATA9		(FLASH_CONTROL_BASE + 0x1154)
#define FCTL_REG_CMDDATA10		(FLASH_CONTROL_BASE + 0x1158)
#define FCTL_REG_CMDDATA11		(FLASH_CONTROL_BASE + 0x115C)
#define FCTL_REG_CMDDATA12		(FLASH_CONTROL_BASE + 0x1160)
#define FCTL_REG_CMDDATA13		(FLASH_CONTROL_BASE + 0x1164)
#define FCTL_REG_CMDDATA14		(FLASH_CONTROL_BASE + 0x1168)
#define FCTL_REG_CMDDATA15		(FLASH_CONTROL_BASE + 0x116C)
#define FCTL_REG_CMDDATA16		(FLASH_CONTROL_BASE + 0x1170)
#define FCTL_REG_CMDDATA17		(FLASH_CONTROL_BASE + 0x1174)
#define FCTL_REG_CMDDATA18		(FLASH_CONTROL_BASE + 0x1178)
#define FCTL_REG_CMDDATA19		(FLASH_CONTROL_BASE + 0x117C)
#define FCTL_REG_CMDDATA20		(FLASH_CONTROL_BASE + 0x1180)
#define FCTL_REG_CMDDATA21		(FLASH_CONTROL_BASE + 0x1184)
#define FCTL_REG_CMDDATA22		(FLASH_CONTROL_BASE + 0x1188)
#define FCTL_REG_CMDDATA23		(FLASH_CONTROL_BASE + 0x118C)
#define FCTL_REG_CMDDATA24		(FLASH_CONTROL_BASE + 0x1190)
#define FCTL_REG_CMDDATA25		(FLASH_CONTROL_BASE + 0x1194)
#define FCTL_REG_CMDDATA26		(FLASH_CONTROL_BASE + 0x1198)
#define FCTL_REG_CMDDATA27		(FLASH_CONTROL_BASE + 0x119C)
#define FCTL_REG_CMDDATA28		(FLASH_CONTROL_BASE + 0x11A0)
#define FCTL_REG_CMDDATA29		(FLASH_CONTROL_BASE + 0x11A4)
#define FCTL_REG_CMDDATA30		(FLASH_CONTROL_BASE + 0x11A8)
#define FCTL_REG_CMDDATA31		(FLASH_CONTROL_BASE + 0x11AC)
#define FCTL_REG_CMDDATAECC0	(FLASH_CONTROL_BASE + 0x11B0)
#define FCTL_REG_CMDDATAECC1	(FLASH_CONTROL_BASE + 0x11B4)
#define FCTL_REG_CMDDATAECC2	(FLASH_CONTROL_BASE + 0x11B8)
#define FCTL_REG_CMDDATAECC3	(FLASH_CONTROL_BASE + 0x11BC)
#define FCTL_REG_CMDDATAECC4	(FLASH_CONTROL_BASE + 0x11C0)
#define FCTL_REG_CMDDATAECC5	(FLASH_CONTROL_BASE + 0x11C4)
#define FCTL_REG_CMDDATAECC6	(FLASH_CONTROL_BASE + 0x11C8)
#define FCTL_REG_CMDDATAECC7	(FLASH_CONTROL_BASE + 0x11CC)
#define FCTL_REG_CMDWEPROTA		(FLASH_CONTROL_BASE + 0x11D0)
#define FCTL_REG_CMDWEPROTB		(FLASH_CONTROL_BASE + 0x11D4)
#define FCTL_REG_CMDWEPROTC		(FLASH_CONTROL_BASE + 0x11D8)
#define FCTL_REG_CMDWEPROTNM	(FLASH_CONTROL_BASE + 0x1210)
#define FCTL_REG_CFGPCNT		(FLASH_CONTROL_BASE + 0x13B4)
#define FCTL_REG_STATCMD		(FLASH_CONTROL_BASE + 0x13D0)
#define FCTL_REG_STATADDR		(FLASH_CONTROL_BASE + 0x13D4)
#define FCTL_REG_STATPCNT		(FLASH_CONTROL_BASE + 0x13D8)

#define MAX_PROTECT_REGION_REGS   3

/* Extract a bitfield helper */
#define EXTRACT_VAL(var, h, l) (((var) & GENMASK((h),(l))) >> (l))

struct mspm0_flash_bank {
	/* Base Address for this instance */
	uint32_t base_address;
	/* chip id register */
	uint32_t did;
	/* Device Unique ID register */
	uint32_t traceid;
	uint8_t version;

	/* Decoded flash information */
	uint32_t data_flash_size_kb;
	uint32_t main_flash_size_kb;
	uint32_t main_flash_num_banks;
	uint32_t sector_size;
	/* Decoded SRAM information */
	uint32_t sram_size_kb;

	/* ID information index */
	uint8_t mspm0_info_index;
	uint8_t mspm0_part_info_index;

	/* Protection register stuff */
	uint32_t protect_reg_base;
	uint32_t protect_reg_count;
};

struct mspm0_part_info {
	const char *partname;
	uint16_t part;
	uint8_t variant;
};

struct mspm0_family_info {
	uint16_t partnum;
	uint8_t part_count;
	const struct mspm0_part_info *part_info;
};

/* https://www.ti.com/lit/ds/symlink/mspm0l1346.pdf Table 8-13 */
static const struct mspm0_part_info mspm0l_parts[] = {
	{ "MSPM0L1105TDGS20R", 0x51DB, 0x16 },
	{ "MSPM0L1105TDGS28R", 0x51DB, 0x83 },
	{ "MSPM0L1105TDYYR", 0x51DB, 0x54 },
	{ "MSPM0L1105TRGER", 0x51DB, 0x86 },
	{ "MSPM0L1105TRHBR", 0x51DB, 0x68 },
	{ "MSPM0L1106TDGS20R", 0x5552, 0x4B },
	{ "MSPM0L1106TDGS28R", 0x5552, 0x98 },
	{ "MSPM0L1106TDYYR", 0x5552, 0x9D },
	{ "MSPM0L1106TRGER", 0x5552, 0x90 },
	{ "MSPM0L1106TRHBR", 0x5552, 0x53 },
	{ "MSPM0L1303SRGER", 0xef0, 0x17 },
	{ "MSPM0L1303TRGER", 0xef0, 0xe2 },
	{ "MSPM0L1304QDGS20R", 0xd717, 0x91 },
	{ "MSPM0L1304QDGS28R", 0xd717, 0xb6 },
	{ "MSPM0L1304QDYYR", 0xd717, 0xa0 },
	{ "MSPM0L1304QRHBR", 0xd717, 0xa9 },
	{ "MSPM0L1304SDGS20R", 0xd717, 0xfa },
	{ "MSPM0L1304SDGS28R", 0xd717, 0x73 },
	{ "MSPM0L1304SDYYR", 0xd717, 0xb7 },
	{ "MSPM0L1304SRGER", 0xd717, 0x26 },
	{ "MSPM0L1304SRHBR", 0xd717, 0xe4 },
	{ "MSPM0L1304TDGS20R", 0xd717, 0x33 },
	{ "MSPM0L1304TDGS28R", 0xd717, 0xa8 },
	{ "MSPM0L1304TDYYR", 0xd717, 0xf9 },
	{ "MSPM0L1304TRGER", 0xd717, 0xb7 },
	{ "MSPM0L1304TRHBR", 0xd717, 0x5a },
	{ "MSPM0L1305QDGS20R", 0x4d03, 0xb7 },
	{ "MSPM0L1305QDGS28R", 0x4d03, 0x74 },
	{ "MSPM0L1305QDYYR", 0x4d03, 0xec },
	{ "MSPM0L1305QRHBR", 0x4d03, 0x78 },
	{ "MSPM0L1305SDGS20R", 0x4d03, 0xc7 },
	{ "MSPM0L1305SDGS28R", 0x4d03, 0x64 },
	{ "MSPM0L1305SDYYR", 0x4d03, 0x91 },
	{ "MSPM0L1305SRGER", 0x4d03, 0x73 },
	{ "MSPM0L1305SRHBR", 0x4d03, 0x2d },
	{ "MSPM0L1305TDGS20R", 0x4d03, 0xa0 },
	{ "MSPM0L1305TDGS28R", 0x4d03, 0xfb },
	{ "MSPM0L1305TDYYR", 0x4d03, 0xde },
	{ "MSPM0L1305TRGER", 0x4d03, 0xea },
	{ "MSPM0L1305TRHBR", 0x4d03, 0x85 },
	{ "MSPM0L1306QDGS20R", 0xbb70, 0x59 },
	{ "MSPM0L1306QDGS28R", 0xbb70, 0xf7 },
	{ "MSPM0L1306QDYYR", 0xbb70, 0x9f },
	{ "MSPM0L1306QRHBR", 0xbb70, 0xc2 },
	{ "MSPM0L1306SDGS20R", 0xbb70, 0xf4 },
	{ "MSPM0L1306SDGS28R", 0xbb70, 0x5 },
	{ "MSPM0L1306SDYYR", 0xbb70, 0xe },
	{ "MSPM0L1306SRGER", 0xbb70, 0x7f },
	{ "MSPM0L1306SRHBR", 0xbb70, 0x3c },
	{ "MSPM0L1306TDGS20R", 0xbb70, 0xa },
	{ "MSPM0L1306TDGS28R", 0xbb70, 0x63 },
	{ "MSPM0L1306TDYYR", 0xbb70, 0x35 },
	{ "MSPM0L1306TRGER", 0xbb70, 0xaa },
	{ "MSPM0L1306TRHBR", 0xbb70, 0x52 },
	{ "MSPM0L1343TDGS20R", 0xb231, 0x2e },
	{ "MSPM0L1344TDGS20R", 0x40b0, 0xd0 },
	{ "MSPM0L1345TDGS28R", 0x98b4, 0x74 },
	{ "MSPM0L1346TDGS28R", 0xf2b5, 0xef },
};

/* https://www.ti.com/lit/ds/symlink/mspm0g3506.pdf Table 8-20 */
static const struct mspm0_part_info mspm0g_parts[] = {
	{ "MSPM0G1105TPTR", 0x8934, 0xD },
	{ "MSPM0G1105TRGZR", 0x8934, 0xFE },
	{ "MSPM0G1106TPMR", 0x477B, 0xD4 },
	{ "MSPM0G1106TPTR", 0x477B, 0x71 },
	{ "MSPM0G1106TRGZR", 0x477B, 0xBB },
	{ "MSPM0G1106TRHBR", 0x477B, 0x0 },
	{ "MSPM0G1107TDGS28R", 0x807B, 0x82 },
	{ "MSPM0G1107TPMR", 0x807B, 0xB3 },
	{ "MSPM0G1107TPTR", 0x807B, 0x32 },
	{ "MSPM0G1107TRGER", 0x807B, 0x79 },
	{ "MSPM0G1107TRGZR", 0x807B, 0x20 },
	{ "MSPM0G1107TRHBR", 0x807B, 0xBC },
	{ "MSPM0G1505SDGS28R", 0x13C4, 0x73 },
	{ "MSPM0G1505SPMR", 0x13C4, 0x53 },
	{ "MSPM0G1505SPTR", 0x13C4, 0x3E },
	{ "MSPM0G1505SRGER", 0x13C4, 0x47 },
	{ "MSPM0G1505SRGZR", 0x13C4, 0x34 },
	{ "MSPM0G1505SRHBR", 0x13C4, 0x30 },
	{ "MSPM0G1506SDGS28R", 0x5AE0, 0x3A },
	{ "MSPM0G1506SPMR", 0x5AE0, 0xF6 },
	{ "MSPM0G1506SRGER", 0x5AE0, 0x67 },
	{ "MSPM0G1506SRGZR", 0x5AE0, 0x75 },
	{ "MSPM0G1506SRHBR", 0x5AE0, 0x57 },
	{ "MSPM0G1507SDGS28R", 0x2655, 0x6D },
	{ "MSPM0G1507SPMR", 0x2655, 0x97 },
	{ "MSPM0G1507SRGER", 0x2655, 0x83 },
	{ "MSPM0G1507SRGZR", 0x2655, 0xD3 },
	{ "MSPM0G1507SRHBR", 0x2655, 0x4D },
	{ "MSPM0G3105SDGS20R", 0x4749, 0x21 },
	{ "MSPM0G3105SDGS28R", 0x4749, 0xDD },
	{ "MSPM0G3105SRHBR", 0x4749, 0xBE },
	{ "MSPM0G3106SDGS20R", 0x54C7, 0xD2 },
	{ "MSPM0G3106SDGS28R", 0x54C7, 0xB9 },
	{ "MSPM0G3106SRHBR", 0x54C7, 0x67 },
	{ "MSPM0G3107SDGS20R", 0xAB39, 0x5C },
	{ "MSPM0G3107SDGS28R", 0xAB39, 0xCC },
	{ "MSPM0G3107SRHBR", 0xAB39, 0xB7 },
	{ "MSPM0G3505SDGS28R", 0xc504, 0x8e },
	{ "MSPM0G3505SPMR", 0xc504, 0x1d },
	{ "MSPM0G3505SPTR", 0xc504, 0x93 },
	{ "MSPM0G3505SRGZR", 0xc504, 0xc7 },
	{ "MSPM0G3505SRHBR", 0xc504, 0xe7 },
	{ "MSPM0G3505TDGS28R", 0xc504, 0xdf },
	{ "MSPM0G3506SDGS28R", 0x151f, 0x8 },
	{ "MSPM0G3506SPMR", 0x151f, 0xd4 },
	{ "MSPM0G3506SPTR", 0x151f, 0x39 },
	{ "MSPM0G3506SRGZR", 0x151f, 0xfe },
	{ "MSPM0G3506SRHBR", 0x151f, 0xb5 },
	{ "MSPM0G3507SDGS28R", 0xae2d, 0xca },
	{ "MSPM0G3507SPMR", 0xae2d, 0xc7 },
	{ "MSPM0G3507SPTR", 0xae2d, 0x3f },
	{ "MSPM0G3507SRGZR", 0xae2d, 0xf7 },
	{ "MSPM0G3507SRHBR", 0xae2d, 0x4c },
};

static const struct mspm0_family_info mspm0_finf[] = {
	{ 0xbb82, ARRAY_SIZE(mspm0l_parts), mspm0l_parts },
	{ 0xbb88, ARRAY_SIZE(mspm0g_parts), mspm0g_parts },
};

/***************************************************************************
*	openocd command interface                                              *
***************************************************************************/

/* flash_bank mspm0 <base> <size> 0 0 <target#>
 */
FLASH_BANK_COMMAND_HANDLER(mspm0_flash_bank_command)
{
	struct mspm0_flash_bank *mspm0_info;

	LOG_ERROR("%s " TARGET_ADDR_FMT, __func__, bank->base);
	switch (bank->base) {
	case FLASH_BASE_NONMAIN:
	case FLASH_BASE_MAIN:
	case FLASH_BASE_DATA:	/* Warning: detected runtime */
		break;
	default:
		LOG_ERROR("Invalid bank address " TARGET_ADDR_FMT, bank->base);
		return ERROR_FAIL;
	}

	mspm0_info = calloc(sizeof(struct mspm0_flash_bank), 1);
	if (!mspm0_info) {
		LOG_ERROR("%s: Out of memory for mspm0_info!", __func__);
		return ERROR_FAIL;
	}

	bank->driver_priv = mspm0_info;

	mspm0_info->mspm0_info_index = 0xff;
	mspm0_info->mspm0_part_info_index = 0xff;
	mspm0_info->sector_size = 0x400;

	return ERROR_OK;
}

/***************************************************************************
*	chip identification and status                                         *
***************************************************************************/

static int get_mspm0_info(struct flash_bank *bank, struct command_invocation *cmd)
{
	struct mspm0_flash_bank *mspm0_info = bank->driver_priv;
	const char *target_name;

	LOG_ERROR("%s " TARGET_ADDR_FMT, __func__, bank->base);
	if (mspm0_info->did == 0)
		return ERROR_FLASH_BANK_NOT_PROBED;

	if (mspm0_info->mspm0_part_info_index == 0xff)
		target_name = "Un Identified";
	else
		target_name =
		    mspm0_finf[mspm0_info->mspm0_info_index].part_info[mspm0_info->
								       mspm0_part_info_index].
		    partname;

	command_print_sameline(cmd,
			       "\nTI MSPM0 information: Chip is "
			       "%s rev %d Device Unique ID: %d\n",
			       target_name, mspm0_info->version, mspm0_info->traceid);
	command_print_sameline(cmd,
			       "main flash: %dKb in %d bank(s), sram: %dKb, data flash: %dKb",
			       mspm0_info->main_flash_size_kb,
			       mspm0_info->main_flash_num_banks, mspm0_info->sram_size_kb,
			       mspm0_info->data_flash_size_kb);

	return ERROR_OK;
}

/* Read device id register, main clock frequency register and fill in driver info structure */
static int mspm0_read_part_info(struct flash_bank *bank)
{
	struct mspm0_flash_bank *mspm0_info = bank->driver_priv;
	struct target *target = bank->target;
	uint32_t did, userid, flashram;
	uint16_t pnum, part;
	uint8_t variant, version;
	const struct mspm0_family_info *minfo = NULL;

	LOG_ERROR("%s " TARGET_ADDR_FMT, __func__, bank->base);
	/* Read and parse chip identification register */
	target_read_u32(target, DID, &did);
	target_read_u32(target, TRACEID, &mspm0_info->traceid);
	target_read_u32(target, USERID, &userid);
	target_read_u32(target, SRAMFLASH, &flashram);
	LOG_DEBUG("did 0x%" PRIx32 ", traceid 0x%" PRIx32 ", userid 0x%" PRIx32
		  ", flashram 0x%" PRIx32 "", did, mspm0_info->traceid, userid, flashram);

	version = EXTRACT_VAL(did, 31, 28);
	pnum = EXTRACT_VAL(did, 27, 12);
	variant = EXTRACT_VAL(userid, 23, 16);
	part = EXTRACT_VAL(userid, 15, 0);
	LOG_DEBUG("Part 0x%" PRIx32 ", Part Num 0x%" PRIx32 ", Variant 0x%" PRIx32
		  ", version 0x%" PRIx32, part, pnum, variant, version);

	/* Valid DIEID? */
	if ((version != 0) && (version != 1)) {
		LOG_WARNING("Unknown Device ID version, cannot identify target");
		return ERROR_FLASH_OPERATION_FAILED;
	}

	/* Check if we at least know the family of devices */
	for (int i = 0; i < (int)ARRAY_SIZE(mspm0_finf); i++) {
		if (mspm0_finf[i].partnum == pnum) {
			mspm0_info->mspm0_info_index = i;
			minfo = &mspm0_finf[i];
			break;
		}
	}
	if (mspm0_info->mspm0_info_index == 0xff) {
		LOG_WARNING("Unsupported DeviceID[0x%" PRIx32 "], cannot identify target",
			    pnum);
		return ERROR_FLASH_OPERATION_FAILED;
	}

	/* Can we specifically identify the chip */
	for (int i = 0; i < minfo->part_count; i++) {
		if (minfo->part_info[i].part == part
		    && minfo->part_info[i].variant == variant) {
			mspm0_info->mspm0_part_info_index = i;
			break;
		}
	}
	if (mspm0_info->mspm0_info_index == 0xff)
		LOG_WARNING("Unsupported PART[0x%" PRIx32 "]/variant[0x%" PRIx32
			    "], known DeviceID[0x%" PRIx32 "]. Attempting to proceed.",
			    part, variant, pnum);
	else
		LOG_DEBUG("Part: %s detected",
			  minfo->part_info[mspm0_info->mspm0_info_index].partname);

	mspm0_info->did = did;
	mspm0_info->version = version;
	mspm0_info->data_flash_size_kb = EXTRACT_VAL(flashram, 31, 26);
	mspm0_info->main_flash_size_kb = EXTRACT_VAL(flashram, 11, 0);
	mspm0_info->main_flash_num_banks = EXTRACT_VAL(flashram, 13, 12) + 1;
	mspm0_info->sram_size_kb = EXTRACT_VAL(flashram, 25, 16);
	LOG_DEBUG("Detected: main flash: %dKb in %d banks, sram: %dKb, data flash: %dKb",
		  mspm0_info->main_flash_size_kb, mspm0_info->main_flash_num_banks,
		  mspm0_info->sram_size_kb, mspm0_info->data_flash_size_kb);

	return ERROR_OK;
}

/***************************************************************************
*	flash operations                                                       *
***************************************************************************/

static int mspm0_protect_reg_mainmap(struct flash_bank *bank, uint32_t sector,
					uint32_t * protect_reg_offset,
					uint32_t * protect_reg_bit)
{
	struct mspm0_flash_bank *mspm0_info = bank->driver_priv;
	uint32_t bank_size, sector_in_bank;

	LOG_ERROR("%s " TARGET_ADDR_FMT, __func__, bank->base);
	if (sector < 32) {
		*protect_reg_offset = 0;
		*protect_reg_bit = sector % 32;
		return ERROR_OK;
	}

	bank_size = mspm0_info->main_flash_size_kb / mspm0_info->main_flash_num_banks;
	sector_in_bank = sector & (bank_size - 1);

	if (sector_in_bank < 256) {
		*protect_reg_offset = 1;
		if (mspm0_info->main_flash_num_banks == 1)
			*protect_reg_bit = BIT((sector_in_bank - 32) / 8);
		else
			*protect_reg_bit = BIT((sector_in_bank) / 8);
		return ERROR_OK;
	}

	if (sector_in_bank >= 512) {
		LOG_ERROR("Invalid sector_in_bank %d at bank " TARGET_ADDR_FMT,
			  sector_in_bank, bank->base);
		return ERROR_FAIL;
	}
	*protect_reg_offset = 2;
	*protect_reg_bit = BIT((sector_in_bank - 256) / 8);
	return ERROR_OK;
}

static int mspm0_protect_reg_map(struct flash_bank *bank, uint32_t sector,
				 uint32_t * protect_reg_offset,
				 uint32_t * protect_reg_bit)
{
	struct mspm0_flash_bank *mspm0_info = bank->driver_priv;

	LOG_ERROR("%s " TARGET_ADDR_FMT, __func__, bank->base);
	switch (bank->base) {
	case FLASH_BASE_NONMAIN:
		*protect_reg_offset = sector / 32;
		*protect_reg_bit = sector % 32;
		break;
	case FLASH_BASE_MAIN:
		int retval;
		retval =
		    mspm0_protect_reg_mainmap(bank, sector, protect_reg_offset,
						 protect_reg_bit);
		if (retval)
			return retval;
		break;
	case FLASH_BASE_DATA:
		LOG_ERROR("Bank protection not available " TARGET_ADDR_FMT, bank->base);
		return ERROR_FAIL;
		break;
	default:
		LOG_ERROR("Invalid bank address " TARGET_ADDR_FMT, bank->base);
		return ERROR_FAIL;
	}

	/* Basic sanity checks */
	if (*protect_reg_offset >= mspm0_info->protect_reg_count) {
		LOG_ERROR("sector %d address overflows protection regs: " TARGET_ADDR_FMT,
			  sector, bank->base);
		return ERROR_FAIL;
	}
	if (*protect_reg_bit >= 32) {
		LOG_ERROR
		    ("sector %d address causes driver algo error for reg bit %d on bank: "
		     TARGET_ADDR_FMT, sector, *protect_reg_bit, bank->base);
		return ERROR_FAIL;
	}

	return ERROR_OK;
}

static int mspm0_protect_check(struct flash_bank *bank)
{
	struct target *target = bank->target;
	struct mspm0_flash_bank *mspm0_info = bank->driver_priv;
	uint32_t protect_reg_cache[MAX_PROTECT_REGION_REGS];
	uint32_t protect_reg_offset, protect_reg_bit;
	unsigned int i;

	LOG_ERROR("%s " TARGET_ADDR_FMT, __func__, bank->base);
	if (mspm0_info->did == 0)
		return ERROR_FLASH_BANK_NOT_PROBED;

	for (i = 0; i < bank->num_sectors; i++)
		bank->sectors[i].is_protected = -1;

	if (!mspm0_info->protect_reg_count)
		return ERROR_OK;

	/* Do a single scan read of regs before we set the status */
	for (i = 0; i < mspm0_info->protect_reg_count; i++) {
		target_read_u32(target,
				mspm0_info->protect_reg_base + (i * 4),
				&protect_reg_cache[i]);
	}

	for (i = 0; i < bank->num_sectors; i++) {
		int retval =
		    mspm0_protect_reg_map(bank, i, &protect_reg_offset, &protect_reg_bit);
		if (retval) {
			bank->sectors[i].is_protected = -1;
			continue;
		}
		bank->sectors[i].is_protected =
		    protect_reg_cache[protect_reg_offset] & BIT(protect_reg_bit);
	}

	return ERROR_OK;
}

static int mspm0_protect(struct flash_bank *bank, int set,
			 unsigned int first, unsigned int last)
{
	struct target *target = bank->target;
	struct mspm0_flash_bank *mspm0_info = bank->driver_priv;
	uint32_t protect_reg_cache[MAX_PROTECT_REGION_REGS];
	uint32_t protect_reg_offset, protect_reg_bit;
	unsigned int i;
	int retval;

	LOG_ERROR("%s " TARGET_ADDR_FMT, __func__, bank->base);

	if (mspm0_info->did == 0)
		return ERROR_FLASH_BANK_NOT_PROBED;

	if (!mspm0_info->protect_reg_count)
		return ERROR_OK;

	/*
	 * Don't trust the protection status set in bank->sectors[i].is_protected
	 * Driver might have changed the flash protection scheme.
	 * So, just rescan and update
	 */

	/* Do a single scan read of regs before we set the status */
	for (i = 0; i < mspm0_info->protect_reg_count; i++) {
		target_read_u32(target,
				mspm0_info->protect_reg_base + (i * 4),
				&protect_reg_cache[i]);
	}
	/* Flip set to binary value */
	set = !!set;
	/* Now set the bits that we need to set with */
	for (i = first; i <= last; i++) {
		retval =
		    mspm0_protect_reg_map(bank, i, &protect_reg_offset, &protect_reg_bit);

		/* Don't proceed unless all OK */
		if (retval)
			return retval;
		if (set)
			protect_reg_cache[protect_reg_offset] |= BIT(protect_reg_bit);
		else
			protect_reg_cache[protect_reg_offset] &= ~BIT(protect_reg_bit);
	}

	for (i = 0; i < mspm0_info->protect_reg_count; i++) {
		target_write_u32(target,
				 mspm0_info->protect_reg_base + (i * 4),
				 protect_reg_cache[i]);
	}

	/*
	 * Update our local state data base, since single bit can protect up to
	 * 8 sectors in some banks
	 */
	for (i = 0; i < bank->num_sectors; i++) {
		retval =
		    mspm0_protect_reg_map(bank, i, &protect_reg_offset, &protect_reg_bit);
		if (retval) {
			bank->sectors[i].is_protected = -1;
			continue;
		}
		bank->sectors[i].is_protected =
		    protect_reg_cache[protect_reg_offset] & BIT(protect_reg_bit);
	}

	return ERROR_OK;
}

static int mspm0_erase(struct flash_bank *bank, unsigned int first, unsigned int last)
{
	LOG_ERROR("%s " TARGET_ADDR_FMT, __func__, bank->base);
	LOG_ERROR("%d - %d", first, last);

	return ERROR_OK;
}

/* see contrib/loaders/flash/mspm0.s for src */

#if 0
static const uint8_t mspm0_write_code[] = {
	/* write: */
	0xDF, 0xF8, 0x40, 0x40,	/* ldr          r4, pFLASH_CTRL_BASE */
	0xDF, 0xF8, 0x40, 0x50,	/* ldr          r5, FLASHWRITECMD */
	/* wait_fifo: */
	0xD0, 0xF8, 0x00, 0x80,	/* ldr          r8, [r0, #0] */
	0xB8, 0xF1, 0x00, 0x0F,	/* cmp          r8, #0 */
	0x17, 0xD0,		/* beq          exit */
	0x47, 0x68,		/* ldr          r7, [r0, #4] */
	0x47, 0x45,		/* cmp          r7, r8 */
	0xF7, 0xD0,		/* beq          wait_fifo */
	/* mainloop: */
	0x22, 0x60,		/* str          r2, [r4, #0] */
	0x02, 0xF1, 0x04, 0x02,	/* add          r2, r2, #4 */
	0x57, 0xF8, 0x04, 0x8B,	/* ldr          r8, [r7], #4 */
	0xC4, 0xF8, 0x04, 0x80,	/* str          r8, [r4, #4] */
	0xA5, 0x60,		/* str          r5, [r4, #8] */
	/* busy: */
	0xD4, 0xF8, 0x08, 0x80,	/* ldr          r8, [r4, #8] */
	0x18, 0xF0, 0x01, 0x0F,	/* tst          r8, #1 */
	0xFA, 0xD1,		/* bne          busy */
	0x8F, 0x42,		/* cmp          r7, r1 */
	0x28, 0xBF,		/* it           cs */
	0x00, 0xF1, 0x08, 0x07,	/* addcs        r7, r0, #8 */
	0x47, 0x60,		/* str          r7, [r0, #4] */
	0x01, 0x3B,		/* subs         r3, r3, #1 */
	0x03, 0xB1,		/* cbz          r3, exit */
	0xE2, 0xE7,		/* b            wait_fifo */
	/* exit: */
	0x00, 0xBE,		/* bkpt         #0 */

	/* pFLASH_CTRL_BASE: */
	0x00, 0xD0, 0x0F, 0x40,	/* .word        0x400FD000 */
	/* FLASHWRITECMD: */
	0x01, 0x00, 0x42, 0xA4	/* .word        0xA4420001 */
};
#endif

static int mspm0_write(struct flash_bank *bank, const uint8_t * buffer,
		       uint32_t offset, uint32_t count)
{
	LOG_ERROR("%s " TARGET_ADDR_FMT, __func__, bank->base);
	return ERROR_OK;
}

static int mspm0_probe(struct flash_bank *bank)
{
	struct mspm0_flash_bank *mspm0_info = bank->driver_priv;
	int retval;
	LOG_ERROR("%s " TARGET_ADDR_FMT, __func__, bank->base);

	/*
	 * If this is a mspm0 chip, it has flash; probe() is just
	 * to figure out how much is present.  Only do it once.
	 */
	if (mspm0_info->did != 0)
		return ERROR_OK;

	/*
	 * mspm0_read_part_info() already handled error checking and
	 * reporting.  Note that it doesn't write, so we don't care about
	 * whether the target is halted or not.
	 */
	retval = mspm0_read_part_info(bank);
	if (retval != ERROR_OK)
		return retval;

	if (bank->sectors) {
		free(bank->sectors);
		bank->sectors = NULL;
	}

	/* provide this for the benefit of the NOR flash framework */
	switch (bank->base) {
	case FLASH_BASE_NONMAIN:
		bank->size = 512;
		bank->num_sectors = 0x1;
		mspm0_info->protect_reg_base = FCTL_REG_CMDWEPROTNM;
		mspm0_info->protect_reg_count = 1;
		break;
	case FLASH_BASE_MAIN:
		bank->size = (mspm0_info->main_flash_size_kb * 1024);
		bank->num_sectors = bank->size / mspm0_info->sector_size;
		mspm0_info->protect_reg_base = FCTL_REG_CMDWEPROTA;
		mspm0_info->protect_reg_count = 3;
		break;
	case FLASH_BASE_DATA:	/* Warning: detected runtime */
		if (!mspm0_info->data_flash_size_kb) {
			LOG_ERROR("Data region NOT available!");
			bank->size = 0x0;
			bank->num_sectors = 0x0;
			return ERROR_OK;
		}
		bank->size = (mspm0_info->main_flash_size_kb * 1024);
		bank->num_sectors = bank->size / mspm0_info->sector_size;
		bank->num_prot_blocks = 0; /* There is no protection here */
		break;
	default:
		LOG_ERROR("Invalid bank address " TARGET_ADDR_FMT, bank->base);
		return ERROR_FAIL;
	}
	bank->sectors = calloc(bank->num_sectors, sizeof(struct flash_sector));
	if (!bank->sectors) {
		LOG_ERROR("%s: Out of memory for sectors!", __func__);
		return ERROR_FAIL;
	}
	for (unsigned int i = 0; i < bank->num_sectors; i++) {
		bank->sectors[i].offset = i * mspm0_info->sector_size;
		bank->sectors[i].size = mspm0_info->sector_size;
		bank->sectors[i].is_erased = -1;
	}

	/* Update with protection information */
	retval = mspm0_protect_check(bank);

	return retval;
}

COMMAND_HANDLER(mspm0_handle_mass_erase_command)
{
	LOG_ERROR("%s ", __func__);
	if (CMD_ARGC < 1)
		return ERROR_COMMAND_SYNTAX_ERROR;

	return ERROR_OK;
}

/**
 * Perform the MSPM0 "Recovering a 'Locked' Device procedure.
 * This performs a mass erase and then restores all nonvolatile registers
 * (including USER_* registers and flash lock bits) to their defaults.
 * Accordingly, flash can be reprogrammed, and JTAG can be used.
 *
 */
COMMAND_HANDLER(mspm0_handle_recover_command)
{
	LOG_ERROR("%s ", __func__);
	if (CMD_ARGC != 0)
		return ERROR_COMMAND_SYNTAX_ERROR;

	return ERROR_OK;
}

static const struct command_registration mspm0_exec_command_handlers[] = {
	{
	 .name = "mass_erase",
	 .usage = "<bank>",
	 .handler = mspm0_handle_mass_erase_command,
	 .mode = COMMAND_EXEC,
	 .help = "erase entire device",
	  },
	{
	 .name = "recover",
	 .handler = mspm0_handle_recover_command,
	 .mode = COMMAND_EXEC,
	 .usage = "",
	 .help = "recover (and erase) locked device",
	  },
	COMMAND_REGISTRATION_DONE
};

static const struct command_registration mspm0_command_handlers[] = {
	{
	 .name = "mspm0",
	 .mode = COMMAND_EXEC,
	 .help = "MSPM0 flash command group",
	 .usage = "",
	 .chain = mspm0_exec_command_handlers,
	  },
	COMMAND_REGISTRATION_DONE
};

const struct flash_driver mspm0_flash = {
	.name = "mspm0",
	.commands = mspm0_command_handlers,
	.flash_bank_command = mspm0_flash_bank_command,
	.erase = mspm0_erase,
	.protect = mspm0_protect,
	.write = mspm0_write,
	.read = default_flash_read,
	.probe = mspm0_probe,
	.auto_probe = mspm0_probe,
	.erase_check = default_flash_blank_check,
	.protect_check = mspm0_protect_check,
	.info = get_mspm0_info,
	.free_driver_priv = default_flash_free_driver_priv,
};
