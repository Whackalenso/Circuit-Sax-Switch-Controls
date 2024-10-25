/*
Nintendo Switch Fightstick - Proof-of-Concept

Based on the LUFA library's Low-Level Joystick Demo
	(C) Dean Camera
Based on the HORI's Pokken Tournament Pro Pad design
	(C) HORI

This project implements a modified version of HORI's Pokken Tournament Pro Pad
USB descriptors to allow for the creation of custom controllers for the
Nintendo Switch. This also works to a limited degree on the PS3.

Since System Update v3.0.0, the Nintendo Switch recognizes the Pokken
Tournament Pro Pad as a Pro Controller. Physical design limitations prevent
the Pokken Controller from functioning at the same level as the Pro
Controller. However, by default most of the descriptors are there, with the
exception of Home and Capture. Descriptor modification allows us to unlock
these buttons for our use.
*/

/** \file
 *
 *  Main source file for the posts printer demo. This file contains the main tasks of
 *  the demo and is responsible for the initial application hardware configuration.
 */

#include "Joystick.h"

// extern const uint8_t image_data[0x12c1] PROGMEM;

// C:\Users\whack\AppData\Local\Arduino15\packages\arduino\tools\avrdude\6.3.0-arduino17/bin/avrdude -CC:\Users\whack\AppData\Local\Arduino15\packages\arduino\tools\avrdude\6.3.0-arduino17/etc/avrdude.conf -v -patmega32u4 -cavr109 -PCOM9 -b57600 -D -Uflash:w:C:\Users\whack\Documents\Arduino\Sketches\Switch_arduino\Mario-Kart\Joystick.hex:i

// Main entry point.
int main(void)
{
	// We'll start by performing hardware and peripheral setup.
	SetupHardware();
	// We'll then enable global interrupts for our use.
	GlobalInterruptEnable();
	// Once that's done, we'll enter an infinite loop.
	for (;;)
	{

		// PORTB = 0b00010000; //PORTD &= ~(1 << PD2);
		//  We need to run our task to process and deliver data for our IN and OUT endpoints.
		HID_Task();
		// We also need to run the main USB management task.
		USB_USBTask();
	}
}

// Configures hardware and peripherals, such as the USB peripherals.
void SetupHardware(void)
{
	// We need to disable watchdog if enabled by bootloader/fuses.
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	// We need to disable clock division before initializing the USB hardware.
	clock_prescale_set(clock_div_1);
	// We can then initialize our hardware and peripherals, including the USB stack.

	// DDRD &= ~(1 << PIND0);
	// DDRD &= ~(1 << PIND1);
	// PORTD = 0b11111111; // enable all pullups

	// DDRB &= ~(1 << PINB4);
	// PORTB = 0b11111111; // enable all pullups

	DDRB = 0x00;
	DDRC = 0x00;
	DDRD = 0x00;
	DDRF = 0x00;
	// DDRF &= ~(1 << PINF1);
	PORTB = 0xFF;
	PORTC = 0xFF;
	PORTD = 0xFF;
	// PORTF |= ((1 << PINF1) | (1 << PINF7));
	PORTF = 0xFF;

	// Select AVcc as the reference voltage
	ADMUX |= (1 << REFS0);
	ADMUX &= ~(1 << REFS1);

	// Set the ADC prescaler to 64 for 125kHz ADC clock (assuming 8MHz CPU clock)
	ADCSRA |= (1 << ADPS2) | (1 << ADPS1);
	ADCSRA &= ~(1 << ADPS0);

	// The USB stack should be initialized last.
	USB_Init();
}

// Fired to indicate that the device is enumerating.
void EVENT_USB_Device_Connect(void)
{
	// We can indicate that we're enumerating here (via status LEDs, sound, etc.).
}

// Fired to indicate that the device is no longer connected to a host.
void EVENT_USB_Device_Disconnect(void)
{
	// We can indicate that our device is not ready (via status LEDs, sound, etc.).
}

// Fired when the host set the current configuration of the USB device after enumeration.
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	// We setup the HID report endpoints.
	ConfigSuccess &= Endpoint_ConfigureEndpoint(JOYSTICK_OUT_EPADDR, EP_TYPE_INTERRUPT, JOYSTICK_EPSIZE, 1);
	ConfigSuccess &= Endpoint_ConfigureEndpoint(JOYSTICK_IN_EPADDR, EP_TYPE_INTERRUPT, JOYSTICK_EPSIZE, 1);

	// We can read ConfigSuccess to indicate a success or failure at this point.
}

// Process control requests sent to the device from the USB host.
void EVENT_USB_Device_ControlRequest(void)
{
	// We can handle two control requests: a GetReport and a SetReport.

	// Not used here, it looks like we don't receive control request from the Switch.
}

// Process and deliver data from IN and OUT endpoints.
void HID_Task(void)
{
	// If the device isn't connected and properly configured, we can't do anything here.
	if (USB_DeviceState != DEVICE_STATE_Configured)
		return;

	// We'll start with the OUT endpoint.
	Endpoint_SelectEndpoint(JOYSTICK_OUT_EPADDR);
	// We'll check to see if we received something on the OUT endpoint.
	if (Endpoint_IsOUTReceived())
	{
		// If we did, and the packet has data, we'll react to it.
		if (Endpoint_IsReadWriteAllowed())
		{
			// We'll create a place to store our data received from the host.
			USB_JoystickReport_Output_t JoystickOutputData;
			// We'll then take in that data, setting it up in our storage.
			while (Endpoint_Read_Stream_LE(&JoystickOutputData, sizeof(JoystickOutputData), NULL) != ENDPOINT_RWSTREAM_NoError)
				;
			// At this point, we can react to this data.

			// However, since we're not doing anything with this data, we abandon it.
		}
		// Regardless of whether we reacted to the data, we acknowledge an OUT packet on this endpoint.
		Endpoint_ClearOUT();
	}

	// We'll then move on to the IN endpoint.
	Endpoint_SelectEndpoint(JOYSTICK_IN_EPADDR);
	// We first check to see if the host is ready to accept data.
	if (Endpoint_IsINReady())
	{
		// We'll create an empty report.
		USB_JoystickReport_Input_t JoystickInputData;
		// We'll then populate this report with what we want to send to the host.
		GetNextReport(&JoystickInputData);
		// Once populated, we can output this data to the host. We do this by first writing the data to the control stream.
		while (Endpoint_Write_Stream_LE(&JoystickInputData, sizeof(JoystickInputData), NULL) != ENDPOINT_RWSTREAM_NoError)
			;
		// We then send an IN packet on this endpoint.
		Endpoint_ClearIN();
	}
}

// #define ECHOES 5
// int echoes = 0;
// USB_JoystickReport_Input_t last_report;

uint16_t ADC_read(uint8_t channel)
{
	// Enable the ADC
	ADCSRA |= (1 << ADEN);

	// Select the ADC channel (0 to 7)
	ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);

	// Start the ADC conversion
	ADCSRA |= (1 << ADSC);

	// Wait until the conversion completes (ADSC becomes 0)
	while (ADCSRA & (1 << ADSC))
		;

	// Read the ADC value (10-bit resolution)
	uint16_t adc_value = ADC;
	return adc_value;
}

bool isPressed(uint8_t port, int pin)
{
	ADCSRA &= ~(1 << ADEN); // Disable ADC to ensure the pin can be used as digital
	return !((port & (1 << pin)) == (1 << pin));
}

// Prepare the next report for the host.
void GetNextReport(USB_JoystickReport_Input_t *const ReportData)
{
	// Prepare an empty report
	memset(ReportData, 0, sizeof(USB_JoystickReport_Input_t));
	int lx = 128;
	int ly = 128;
	ReportData->HAT = HAT_CENTER;

	//// Repeat ECHOES times the last report
	// if (echoes > 0) // if this, dont return but make the last lines not happen
	//{
	//	memcpy(ReportData, &last_report, sizeof(USB_JoystickReport_Input_t));
	//	echoes--;
	//	return;
	// // }

	if (isPressed(PIND, PIND1)) // FF
	{
		ReportData->Button |= SWITCH_START;
	}
	if (isPressed(PINB, PINB5)) // SK1
	{
		ReportData->Button |= SWITCH_SELECT;
	}

	if (isPressed(PIND, PIND3)) // D1 - K6
	{
		ReportData->Button |= SWITCH_B;
	}
	if (isPressed(PIND, PIND2)) // OCT
	{
		ReportData->Button |= SWITCH_X;
	}
	if (isPressed(PINB, PINB1)) // SCK - K3
	{
		ReportData->Button |= SWITCH_A;
	}

	if (isPressed(PINB, PINB7)) // D11 - SK3
	{
		ReportData->Button |= SWITCH_R;
	}
	// if (isPressed(PINB, PINB6)) // D10 - SK2
	// {
	// 	ReportData->Button |= SWITCH_R;
	// }
	if (isPressed(PIND, PIND4)) // D4 - P1
	{
		ReportData->Button |= SWITCH_L;
	}
	if (isPressed(PINF, PINF1)) // A4 - Eb
	{
		ReportData->Button |= SWITCH_ZR;
	}
	if (isPressed(PINF, PINF7)) // A0 - G#
	{
		ReportData->Button |= SWITCH_ZL;
	}

	// Left Joystick
	if (isPressed(PIND, PIND0)) // K1
	{
		if (isPressed(PINB, PINB3))
		{ // A3 - Bb
			ReportData->HAT = HAT_RIGHT;
		}
		else
		{
			lx = 255;
		}
	}
	if (isPressed(PINB, PINB4)) // K2
	{
		if (isPressed(PINB, PINB3))
		{ // A3 - Bb
			ReportData->HAT = HAT_LEFT;
		}
		else
		{
			lx = lx == 255 ? 128 : 0;
		}
	}
	if (isPressed(PINC, PINC7)) // D13 - K4
	{
		if (isPressed(PINB, PINB3))
		{ // A3 - Bb
			ReportData->HAT = HAT_TOP;
		}
		else
		{
			ly = 0;
		}
	}
	else if (isPressed(PIND, PIND6)) // D12 - K5
	{
		if (isPressed(PINB, PINB3))
		{ // A3 - Bb
			ReportData->HAT = HAT_BOTTOM;
		}
		else
		{
			ly = ly == 0 ? 128 : 255;
		}
	}

	uint16_t breath_val = ADC_read(0);
	if (breath_val > 100)
	{
		ReportData->Button |= SWITCH_A;
	}

	ReportData->LX = lx;
	ReportData->LY = ly;
	ReportData->RX = 128;
	ReportData->RY = 128;

	//// Prepare to echo this report
	// memcpy(&last_report, ReportData, sizeof(USB_JoystickReport_Input_t));
	// echoes = ECHOES;
}