/* Copyright (c) 2015-2017 Henrik Brix Andersen <henrik@brixandersen.dk>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdio.h>
#include <string.h>

#include <mb_interface.h>
#include <xil_cache.h>
#include <xil_printf.h>
#include <xspi.h>
#include <xparameters.h>
#include <xhwicap.h>
#include "xuartlite_l.h"
#define VERBOSE
//#define DEBUG_ELF_LOADER
//#define JUMP2GOLDEN

//#ifdef VERBOSE
//#include "xstatus.h"
//#include "xgpio.h"
//#endif // VERBOSE

#include "elf32.h"
#include "eb-config.h"

#define XUartLite_IsTransmitEmpty(BaseAddress) \
	((XUartLite_GetStatusReg((BaseAddress)) & XUL_SR_TX_FIFO_EMPTY) == \
		XUL_SR_TX_FIFO_EMPTY)

static XSpi Spi;
static u8 ReadBuffer[EFFECTIVE_READ_BUFFER_SIZE + SPI_VALID_DATA_OFFSET];
//#ifdef VERBOSE
//static XGpio AXI_gpio_inst;
//#endif // VERBOSE
/*
 * Reduce code size on Microblaze (no interrupts used)
 */
#ifdef __MICROBLAZE__
void _interrupt_handler() {}
void _exception_handler() {}
void _hw_exception_handler() {}
#endif

#if !defined (QFLASH_LE_16MB)
void FlashEnterExit4BAddMode(XSpi *QspiPtr, unsigned int Enable)
{
	u8 WriteDisableCmd = { WRITE_DISABLE_CMD };
	u8 WriteEnableCmd = { WRITE_ENABLE_CMD };
	u8 Cmd = { ENTER_4B_ADDR_MODE };

	if(Enable) {
		Cmd = ENTER_4B_ADDR_MODE;
	} else {
		Cmd = EXIT_4B_ADDR_MODE;
	}

	XSpi_Transfer(QspiPtr, &WriteEnableCmd, NULL, sizeof(WriteEnableCmd));
	XSpi_Transfer(QspiPtr, &Cmd, NULL, sizeof(Cmd));
	XSpi_Transfer(QspiPtr, &WriteDisableCmd, NULL, sizeof(WriteDisableCmd));
}
#endif // #if !defined (QFLASH_LE_16MB)

/**
 * Simple wrapper function for reading a number of bytes from SPI flash.
 */
int spi_flash_read(XSpi *InstancePtr, u32 FlashAddress, u8 *RecvBuffer, unsigned int ByteCount)
{
#if defined (QFLASH_LE_16MB)
	RecvBuffer[0] = SPI_READ_OPERATION;
	RecvBuffer[1] = (FlashAddress >> 16) & 0xFF;
	RecvBuffer[2] = (FlashAddress >> 8) & 0xFF;
	RecvBuffer[3] = FlashAddress & 0xFF;
#else
	RecvBuffer[0] = SPI_READ_OPERATION_4B;
	RecvBuffer[1] = (FlashAddress >> 24) & 0xFF;	
	RecvBuffer[2] = (FlashAddress >> 16) & 0xFF;
	RecvBuffer[3] = (FlashAddress >> 8) & 0xFF;
	RecvBuffer[4] = FlashAddress & 0xFF;
#endif // 	

#if defined (QFLASH_LE_16MB)
		FlashEnterExit4BAddMode(&Spi, 0);
#endif // #if defined (QFLASH_LE_16MB)

	return XSpi_Transfer(InstancePtr, RecvBuffer, RecvBuffer, ByteCount + SPI_VALID_DATA_OFFSET);
}

//#ifdef VERBOSE
//int xgpio_init(void)
//{
//	int Status ;
//
//	u32 ret;
//
//	Status = XGpio_Initialize(&AXI_gpio_inst, XPAR_XGPIO_I2C_0_AXI_GPIO_0_DEVICE_ID) ;
//	if (Status != XST_SUCCESS)
//	{
//		return XST_FAILURE ;
//	}
//	/* set as output */
////	XGpio_DiscreteWrite(&AXI_gpio_inst, 1, 0xfff);
////	XGpio_SetDataDirection(&AXI_gpio_inst, 1, 0x0);
////	XGpio_DiscreteWrite(&AXI_gpio_inst, 1, 0xfff);
//	XGpio_SetDataDirection(&AXI_gpio_inst, 2, 0x0);
//	XGpio_DiscreteWrite(&AXI_gpio_inst, 2, 1);
//
//	return XST_SUCCESS ;
//}
//#endif // VERBOSE


/*****************************************************************************/
/**
*
* This function issues IPROG to the target device.
*
* @param	iprog_address is the Jump address for the ICAP
* @param	BaseAddress is the base address of the HwIcap instance.
* @return	XST_SUCCESS if successful, otherwise XST_FAILURE
*
* @note		None
*
******************************************************************************/
int WATCHDOG_TIMER_CFG(u32 timer_value)
{
//	int Status;
	u32 Index;
	u32 Retries;

	// ref to xapp1247 table 3
	u32 SendtoFIFO[12] =
	{
		0xFFFFFFFF, /* Dummy Word */
		0x000000BB, /* Bus width auto detect, word 1 */
		0x11220044, /* Bus width auto detect, word 2 */
		0xFFFFFFFF, /* Dummy Word */
		0xFFFFFFFF, /* Dummy Word */
		0xAA995566, /* Sync Word*/
		0x20000000, /* Type 1 NO OP */
		0x20000000, /* Type 1 NO OP  */
		0x30022001, /* Packet Type 1 command: Write TIMER register */
		0x00FFFFFF, /* Timer Register value. This enables TIMER_CFG and sets the TIMER value */
		0x20000000, /* Type 1 NO OP  */
		0x20000000, /* Type 1 NO OP  */
	};
	SendtoFIFO[9] = timer_value; //Load the Warm Boot Address


	/*
	 * Write command sequence to the FIFO
	 */
	for (Index = 0; Index < 12; Index++) {
		XHwIcap_WriteReg(XPAR_HWICAP_0_BASEADDR, XHI_WF_OFFSET, SendtoFIFO[Index]);
	}

	/*
	 * Start the transfer of the data from the FIFO to the ICAP device.
	 */
	XHwIcap_WriteReg(XPAR_HWICAP_0_BASEADDR, XHI_CR_OFFSET, XHI_CR_WRITE_MASK);

	/*
	 * Poll for done, which indicates end of transfer
	 */
	Retries = 0;
	while ((XHwIcap_ReadReg(XPAR_HWICAP_0_BASEADDR, XHI_SR_OFFSET) &
			XHI_SR_DONE_MASK) != XHI_SR_DONE_MASK) {
		Retries++;
		if (Retries > XHI_MAX_RETRIES) {

			/*
			 * Waited to long. Exit with error.
			 */
			printf("\r\nHwIcapLowLevelExample failed- retries  \
			failure. \r\n\r\n");

			return XST_FAILURE;
		}
	}

	/*
	 * Wait till the Write bit is cleared in the CR register.
	 */
	while ((XHwIcap_ReadReg(XPAR_HWICAP_0_BASEADDR, XHI_CR_OFFSET)) &
					XHI_CR_WRITE_MASK);
	/*
	 * Write to the SIZE register. We want to readback one word.
	 */
	XHwIcap_WriteReg(XPAR_HWICAP_0_BASEADDR, XHI_SZ_OFFSET, 1);


	/*
	 * Start the transfer of the data from ICAP to the FIFO.
	 */
	XHwIcap_WriteReg(XPAR_HWICAP_0_BASEADDR, XHI_CR_OFFSET, XHI_CR_READ_MASK);

	/*
	 * Poll for done, which indicates end of transfer
	 */
	Retries = 0;
	while ((XHwIcap_ReadReg(XPAR_HWICAP_0_BASEADDR, XHI_SR_OFFSET) &
			XHI_SR_DONE_MASK) != XHI_SR_DONE_MASK) {
		Retries++;
		if (Retries > XHI_MAX_RETRIES) {

			/*
			 * Waited too long. Exit with error.
			 */

			return XST_FAILURE;
		}
	}


	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function issues IPROG to the target device.
*
* @param	iprog_address is the Jump address for the ICAP
* @param	BaseAddress is the base address of the HwIcap instance.
* @return	XST_SUCCESS if successful, otherwise XST_FAILURE
*
* @note		None
*
******************************************************************************/
int ISSUE_IPROG(u32 iprog_address)
{
//	int Status;
	u32 Index;
	u32 Retries;

	u32 SendtoFIFO[8] =
	{
		0xFFFFFFFF, /* Dummy Word */
		0xAA995566, /* Sync Word*/
		0x20000000, /* Type 1 NO OP */
		0x30020001, /* Write WBSTAR cmd */
		0x00800000, /* Warm boot start address (Load the desired address) */
		0x30008001, /* Write CMD */
		0x0000000F, /* Write IPROG */
		0x20000000, /* Type 1 NO OP  */
	};
	SendtoFIFO[4] = iprog_address; //Load the Warm Boot Address


	/*
	 * Write command sequence to the FIFO
	 */
	for (Index = 0; Index < 8; Index++) {
		XHwIcap_WriteReg(XPAR_HWICAP_0_BASEADDR, XHI_WF_OFFSET, SendtoFIFO[Index]);
	}

	/*
	 * Start the transfer of the data from the FIFO to the ICAP device.
	 */
	XHwIcap_WriteReg(XPAR_HWICAP_0_BASEADDR, XHI_CR_OFFSET, XHI_CR_WRITE_MASK);

	/*
	 * Poll for done, which indicates end of transfer
	 */
	Retries = 0;
	while ((XHwIcap_ReadReg(XPAR_HWICAP_0_BASEADDR, XHI_SR_OFFSET) &
			XHI_SR_DONE_MASK) != XHI_SR_DONE_MASK) {
		Retries++;
		if (Retries > XHI_MAX_RETRIES) {

			/*
			 * Waited to long. Exit with error.
			 */
			printf("\r\nHwIcapLowLevelExample failed- retries  \
			failure. \r\n\r\n");

			return XST_FAILURE;
		}
	}

	/*
	 * Wait till the Write bit is cleared in the CR register.
	 */
	while ((XHwIcap_ReadReg(XPAR_HWICAP_0_BASEADDR, XHI_CR_OFFSET)) &
					XHI_CR_WRITE_MASK);
	/*
	 * Write to the SIZE register. We want to readback one word.
	 */
	XHwIcap_WriteReg(XPAR_HWICAP_0_BASEADDR, XHI_SZ_OFFSET, 1);


	/*
	 * Start the transfer of the data from ICAP to the FIFO.
	 */
	XHwIcap_WriteReg(XPAR_HWICAP_0_BASEADDR, XHI_CR_OFFSET, XHI_CR_READ_MASK);

	/*
	 * Poll for done, which indicates end of transfer
	 */
	Retries = 0;
	while ((XHwIcap_ReadReg(XPAR_HWICAP_0_BASEADDR, XHI_SR_OFFSET) &
			XHI_SR_DONE_MASK) != XHI_SR_DONE_MASK) {
		Retries++;
		if (Retries > XHI_MAX_RETRIES) {

			/*
			 * Waited too long. Exit with error.
			 */

			return XST_FAILURE;
		}
	}


	return XST_SUCCESS;
}

int main()
{
	elf32_hdr hdr;
	elf32_phdr phdr;
	u32 addr;
	int ret, i, j;
	u8 timeout = 0;

	/*
	 * Disable caches
	 */
#if (XPAR_MICROBLAZE_USE_DCACHE == 1)
	Xil_DCacheInvalidate();
	Xil_DCacheDisable();
#endif
#if (XPAR_MICROBLAZE_USE_ICACHE == 1)
	Xil_ICacheInvalidate();
	Xil_ICacheDisable();
#endif

#ifdef VERBOSE
//	xgpio_init();
	usleep(100000);
	print("\r\nSPI ELF Bootloader\r\n");
#endif

	// OK
//	while(!XUartLite_IsTransmitEmpty(STDOUT_BASEADDRESS));
//	WATCHDOG_TIMER_CFG(0x4000000F);
//	ISSUE_IPROG(0);

	/*
	 * Initialize the SPI controller in polled mode
	 * and enable the flash slave select
	 */
	ret = XSpi_Initialize(&Spi, SPI_DEVICE_ID);
	if (ret != XST_SUCCESS) {
		if (ret == XST_DEVICE_IS_STARTED) {
			if (XSpi_Stop(&Spi) == XST_DEVICE_BUSY) {
#ifdef VERBOSE
				print("SPI device is busy\r\n");
#endif
				return -1;
			}
		} else if (ret == XST_DEVICE_NOT_FOUND) {
#ifdef VERBOSE
			print("SPI device not found\r\n");
#endif
			return -1;
		} else {
#ifdef VERBOSE
			print("Unknown error\r\n");
#endif
			return -1;
		}
	}

	ret = XSpi_SetOptions(&Spi, XSP_MASTER_OPTION | XSP_MANUAL_SSELECT_OPTION);
	if (ret != XST_SUCCESS) {
		if (ret == XST_DEVICE_BUSY) {
#ifdef VERBOSE
			print("SPI device is busy\r\n");
#endif
			return -1;
		} else if (ret == XST_SPI_SLAVE_ONLY) {
#ifdef VERBOSE
			print("SPI device is slave-only\r\n");
#endif
			return -1;
		} else {
#ifdef VERBOSE
			print("Unknown error\r\n");
#endif
			return -1;
		}
	}

	ret = XSpi_SetSlaveSelect(&Spi, SPI_FLASH_SLAVE_SELECT);
	if (ret != XST_SUCCESS) {
		if (ret == XST_DEVICE_BUSY) {
#ifdef VERBOSE
			print("SPI device is busy\r\n");
#endif
			return ret;
		} else if (ret == XST_SPI_TOO_MANY_SLAVES) {
#ifdef VERBOSE
			print("Too many SPI slave devices\r\n");
#endif
			return ret;
		} else {
#ifdef VERBOSE
			print("Unknown error\r\n");
#endif
			return ret;
		}
	}

	// OK
//	while(!XUartLite_IsTransmitEmpty(STDOUT_BASEADDRESS));
//	WATCHDOG_TIMER_CFG(0x4000000F);
//	ISSUE_IPROG(0);

	XSpi_Start(&Spi);
	XSpi_IntrGlobalDisable(&Spi);

#ifdef VERBOSE
	print("Copying ELF image from SPI flash @ 0x");
	putnum(ELF_IMAGE_BASEADDR);
	print(" to RAM\r\n");
#endif

	// OK
//	while(!XUartLite_IsTransmitEmpty(STDOUT_BASEADDRESS));
//	WATCHDOG_TIMER_CFG(0x4000000F);
//	ISSUE_IPROG(0);

#if !defined (QFLASH_LE_16MB)
    FlashEnterExit4BAddMode(&Spi, 1);
#endif // #if !defined (QFLASH_LE_16MB)

	usleep(100*1000);
	/*
	 * Read ELF header
	 */
	print(" sizeof(hdr) = ");
	putnum(sizeof(hdr));
	print("\r\n");

	// NG
//	while(!XUartLite_IsTransmitEmpty(STDOUT_BASEADDRESS));
//	WATCHDOG_TIMER_CFG(0x4000000F);
//	ISSUE_IPROG(0);

	do
	{
		usleep(100*1000);
		ret = spi_flash_read(&Spi, ELF_IMAGE_BASEADDR, ReadBuffer, sizeof(hdr));
		if (ret == XST_SUCCESS)
		{
			memcpy(&hdr, ReadBuffer + SPI_VALID_DATA_OFFSET, sizeof(hdr));

#ifdef DEBUG_ELF_LOADER
			print("hdr.ident:\r\n");
			for (i = 0; i < HDR_IDENT_NBYTES; i++) 
			{
				putnum(hdr.ident[i]);
				print("\r\n");
			}
#endif
		}
		else
		{
#ifdef VERBOSE
			print("Failed to read ELF header");
#endif
			//return -1;
		}
		
		if(timeout>=20)
		{
		    break;
		}
		timeout++;
	}
	while(hdr.ident[0] != HDR_IDENT_MAGIC_0 ||
			hdr.ident[1] != HDR_IDENT_MAGIC_1 ||
			hdr.ident[2] != HDR_IDENT_MAGIC_2 ||
			hdr.ident[3] != HDR_IDENT_MAGIC_3);

	/*
	 * Validate ELF header
	 */
	if (hdr.ident[0] != HDR_IDENT_MAGIC_0 ||
			hdr.ident[1] != HDR_IDENT_MAGIC_1 ||
			hdr.ident[2] != HDR_IDENT_MAGIC_2 ||
			hdr.ident[3] != HDR_IDENT_MAGIC_3)
	{
#ifdef VERBOSE
		print("Invalid ELF header");
#endif

#if !defined (QFLASH_LE_16MB)
		FlashEnterExit4BAddMode(&Spi, 0);
#endif // #if !defined (QFLASH_LE_16MB)

#ifdef VERBOSE
		while(!XUartLite_IsTransmitEmpty(STDOUT_BASEADDRESS));
#endif // defined VERBOSE

#ifdef JUMP2GOLDEN
		WATCHDOG_TIMER_CFG(0x4000000F);
		ISSUE_IPROG(0);
#endif // defined JUMP2GOLDEN
		
		return -1;
	}

	/**
	 * Read ELF program headers
	 */
	for (i = 0; i < hdr.phnum; i++) {
		ret = spi_flash_read(&Spi, ELF_IMAGE_BASEADDR + hdr.phoff + i * sizeof(phdr), ReadBuffer, sizeof(phdr));
		if (ret == XST_SUCCESS) {
			memcpy(&phdr, ReadBuffer + SPI_VALID_DATA_OFFSET, sizeof(phdr));

			if (phdr.type == PHDR_TYPE_LOAD) {
				/*
				 * Copy program segment from flash to RAM
				 */
				for (addr = 0; addr < phdr.filesz; addr += EFFECTIVE_READ_BUFFER_SIZE) {
					if (addr + EFFECTIVE_READ_BUFFER_SIZE > phdr.filesz) {
						ret = spi_flash_read(&Spi, ELF_IMAGE_BASEADDR + phdr.offset + addr,
								ReadBuffer, phdr.filesz - addr);
#ifdef DEBUG_ELF_LOADER
						print("End of section: ");
						putnum(i);
						print("\r\n");

						print("filesz: ");
						putnum(phdr.filesz);
						print("\r\n");

						print("addr: ");
						putnum(addr);
						print("\r\n");

						print("segment end:\r\n");
						for (j = 15; j >= 0; j--) {
							putnum(ReadBuffer[SPI_VALID_DATA_OFFSET + phdr.filesz - addr - j]);
							print(" ");
						}
						print("\r\n");
#endif

					} else {
						ret = spi_flash_read(&Spi, ELF_IMAGE_BASEADDR + phdr.offset + addr,
								ReadBuffer, EFFECTIVE_READ_BUFFER_SIZE);
#ifdef DEBUG_ELF_LOADER
						if (addr == 0) {
							print("segment start:\r\n");
							for (j = 0; j < EFFECTIVE_READ_BUFFER_SIZE; j++) {
								putnum(ReadBuffer[SPI_VALID_DATA_OFFSET + j]);
								print(" ");
							}
							print("\r\n");
						}
#endif
					}

					if (ret == XST_SUCCESS) {
						if (addr + EFFECTIVE_READ_BUFFER_SIZE > phdr.filesz) {
							memcpy((void*)phdr.paddr + addr, ReadBuffer + SPI_VALID_DATA_OFFSET, phdr.filesz - addr);
						} else {
							memcpy((void*)phdr.paddr + addr, ReadBuffer + SPI_VALID_DATA_OFFSET, EFFECTIVE_READ_BUFFER_SIZE);
						}

						if (addr % 1024 == 0) {
#ifdef VERBOSE
							print(".");
#endif
						}
					} else {
#ifdef VERBOSE
						print("Failed to read ELF program segment");
#endif
						return -1;
					}
				}

				/*
				 * Fill remaining segment in RAM with zeros
				 */
				if (phdr.memsz > phdr.filesz) {
					memset((void*)(phdr.paddr + phdr.filesz), 0, phdr.memsz - phdr.filesz);
				}
			}
		} else {
#ifdef VERBOSE
			print("Failed to read ELF program header");
#endif
			return -1;
		}
	}

	/**
	 * Jump to ELF entry address
	 */
#ifdef VERBOSE
	print("\r\nTransferring execution to program @ 0x");
	putnum(hdr.entry);
	print("\r\n");
#endif
	((void (*)())hdr.entry)();

	// Never reached
	return 0;
}
