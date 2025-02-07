#include <kernel/kernel.h>

void print(const char* str);
void printLn(void);

char ConsoleGetChar(void);
void ConsoleRunCommand(void);
void ConsoleClear(char clearToChar);
void ConsoleBackspace(void);

#define KEYBOARD_STRING_LENGTH_MAX  64

char keyboard_string[KEYBOARD_STRING_LENGTH_MAX];
uint8_t keyboard_string_length = 0;
char keyboard_char[] = {' ', '\0'};
uint8_t console_prompt_length = 3;

char console_prompt[16] = {'A', '>', '\0'};

struct Bus bus;



int main(void) {
    
    // Zero the system bus
    bus_initiate();
    
    // Allow board some time to stabilize
    _delay_ms(1000);
    
    bus.read_waitstate = 20;
    bus.write_waitstate = 20;
    
    print(console_prompt);
    
    keyboard_char[0] = ConsoleGetChar();
    
    while(1) {
        
        // Get keyboard character
        char key = ConsoleGetChar();
        
        // Check new input from the keyboard
        if (keyboard_char[0] == key) 
            continue;
        keyboard_char[0] = key;
        
        // Return
        if (key == '\n') {
            
            printLn();
            
            ConsoleRunCommand();
            
            // Zero the keyboard string
            keyboard_string[0] = '\0';
            keyboard_string_length=0;
            
            print(console_prompt);
            
            // Testing - Print keyboard string
            //print(keyboard_string);
            
            continue;
        }
        
        // Backspace
        if (key == 0x01) {
            ConsoleBackspace();
            continue;
        }
        
        //if (keyboard_string_length > KEYBOARD_STRING_LENGTH_MAX - 2) 
        //    continue;
        
        keyboard_string[keyboard_string_length] = keyboard_char[0];
        keyboard_string_length++;
        
        print(&keyboard_string[keyboard_string_length-1]);
        
        //keyboard_string[keyboard_string_length] = '\0';
        
    }
    
    return 1;
}






uint8_t console_line = 0;
uint8_t console_position = 0;
uint8_t displayRows = 0;
uint8_t displayCols = 0;


void print(const char* str) {
    
    for (uint16_t i=0; i < 0xffff; i++) {
        if (str[i] == '\0') 
            break;
        
        if (str[i] == '\n') {
            console_line++;
            if (console_line > 7) 
                printLn();
            
            console_position=0;
            bus_write_io(&bus, 0x40001, console_line); // line
            bus_write_io(&bus, 0x40002, console_position); // Pos
            return;
        }
        
        bus_write_io(&bus, 0x40002, console_position); // Pos
        console_position++;
        bus_write_io(&bus, 0x4000a, str[i]); // char memory
        
        if (console_position > 20) {
            console_position=0;
            console_line++;
            
            if (console_line > 7) 
                printLn();
            
            bus_write_io(&bus, 0x40001, console_line); // line
        }
        
    }
    
    bus_write_io(&bus, 0x40002, console_position); // Pos
    return;
}

void printLn(void) {
    if (console_line < 7) {
        console_line++;
        bus_write_io(&bus, 0x40001, console_line); // line
    } else {
        bus_write_io(&bus, 0x40005, 0x01); // Newline
        console_line = 7;
        // Wait for the display to shift up the charactors
        _delay_ms(30);
    }
    console_position = 0;
    
    bus_write_io(&bus, 0x40001, console_line); // line
    bus_write_io(&bus, 0x40002, console_position); // Pos
    return;
}

void ConsoleClear(char clearToChar) {
    
    for (uint8_t l=0; l < 8; l++) {
        
        bus_write_io(&bus, 0x40001, l); // line
        bus_write_io(&bus, 0x40002, 0); // Pos
        
        for (uint8_t c=0; c < 21; c++) 
            bus_write_io(&bus, 0x4000a + c, clearToChar);
        
    }
    
    bus_write_io(&bus, 0x40001, 0); // line
    bus_write_io(&bus, 0x40002, 0); // Pos
    
    console_line = 0;
    console_position = 0;
    return;
}

void ConsoleBackspace(void) {
    if (console_position < console_prompt_length) 
        return;
    keyboard_string[keyboard_string_length] = '\0';
    console_position--;
    bus_write_io(&bus, 0x40002, console_position); // Pos
    print((const char[]){' ', '\0'});
    console_position--;
    keyboard_string_length--;
    bus_write_io(&bus, 0x40002, console_position); // Pos
    return;
}


void ConsoleRunCommand(void) {
    
    const char* commandCLS = "cls";
    
    // Find function
    //char* command = strstr(keyboard_string, commandCLS);
    uint8_t passed = 1;
    for (uint8_t i=0; i < sizeof(commandCLS); i++) {
        if (commandCLS[i] == keyboard_string[i]) 
            continue;
        // Wrong function
        passed = 0;
        return;
    }
    
    // Check command name passed
    if (passed == 1) {
        ConsoleClear(' ');
        print(console_prompt);
    }
    
    return;
}










// Decode the scan code returning an ASCII character
char kbDecodeScanCode(uint8_t scancode_low, uint8_t scancode_high) {
    
	if ( (scancode_low == 0x98)|(scancode_low == 0x99)|(scancode_low == 0x92)|(scancode_low == 0x91)|(scancode_low == 0x90)|(scancode_low == 0x9a)|(scancode_low == 0x9b) ) {
		if (scancode_high == 0xc4) {return 0x11;} // Left shift pressed
	}
	
	if ( (scancode_low == 0x58)|(scancode_low == 0x59)|(scancode_low == 0x52)|(scancode_low == 0x51)|(scancode_low == 0x50)|(scancode_low == 0x5a)|(scancode_low == 0x5b) ) {
		if (scancode_high == 0xd6) {return 0x11;} // Right shift pressed
	}
	
	if (scancode_low == 0xdf) {
		if (scancode_high == 0x9a) {return 0x05;} // Left arrow
		if (scancode_high == 0x96) {return ']';}  // Right square bracket
		if (scancode_high == 0x90) {return 'i';}
		if (scancode_high == 0xc6) {return 's';}
		if (scancode_high == 0x88) {return 'd';}
		if (scancode_high == 0xca) {return 'f';}
		if (scancode_high == 0xcc) {return 'h';}
		if (scancode_high == 0x8e) {return 'j';}
		if (scancode_high == 0xd2) {return 'l';}
		return  0x00;
	}
	
	if (scancode_low == 0x9f) {
		if (scancode_high == 0xc4) {return 0x12;} // Shift left released
		if (scancode_high == 0xd9) {return 0x01;} // Backspace
		if (scancode_high == 0xd6) {return '\n';} // Enter
		if (scancode_high == 0xdc) {return 0x04;} // Down arrow
		if (scancode_high == 0xd3) {return '-';}  // Dash
		if (scancode_high == 0x83) {return '`';}  // Apostrophe
		if (scancode_high == 0x9d) {return 0x07;} // Escape
		if (scancode_high == 0x92) {return '/';}  // Forward slash
		if (scancode_high == 0x94) {return 0x27;} // '
		if (scancode_high == 0x91) {return '9';}
		if (scancode_high == 0x8f) {return '8';}
		if (scancode_high == 0xcd) {return '6';}
		if (scancode_high == 0xcb) {return '5';}
		if (scancode_high == 0x89) {return '3';}
		if (scancode_high == 0xc7) {return '2';}
		if (scancode_high == 0x85) {return '1';}
		if (scancode_high == 0x86) {return 'z';}
		if (scancode_high == 0xc8) {return 'x';}
		if (scancode_high == 0x8a) {return 'v';}
		if (scancode_high == 0x8c) {return 'b';}
		if (scancode_high == 0xce) {return 'm';}
		if (scancode_high == 0xd0) {return 'k';}
		return  0x00;
	}
	
	if (scancode_low == 0x5f) {
		if (scancode_high == 0xd6) {return 0x12;} // Right shift released
		if (scancode_high == 0x9d) {return 0x03;} // Up arrow
		if (scancode_high == 0x8a) {return 0x20;} // Space
		if (scancode_high == 0xc4) {return 0x09;} // Alt
		if (scancode_high == 0xd5) {return '=';}  // Equals
		if (scancode_high == 0xdc) {return 0x10;} // Delete
		if (scancode_high == 0xd0) {return ',';}  // Comma
		if (scancode_high == 0x92) {return '.';}  // Period
		if (scancode_high == 0x97) {return 0x5c;} // Backslash
		if (scancode_high == 0x91) {return '0';}
		if (scancode_high == 0x8f) {return '7';}
		if (scancode_high == 0x89) {return '4';}
		if (scancode_high == 0xc8) {return 'c';}
		if (scancode_high == 0x8c) {return 'n';}
		if (scancode_high == 0x85) {return 'q';}
		if (scancode_high == 0xc7) {return 'w';}
		if (scancode_high == 0xcb) {return 'r';}
		if (scancode_high == 0xcd) {return 'y';}
		if (scancode_high == 0xd3) {return 'p';}
		return  0x00;
	}
	
	if (scancode_low == 0x1f) {
		if (scancode_high == 0xdd) {return 0x06;} // Right arrow
		if (scancode_high == 0xc5) {return 0x08;} // Control
		if (scancode_high == 0x93) {return ';';}  // Semi-colon
		if (scancode_high == 0x95) {return '[';}  // Left square bracket
		if (scancode_high == 0xc9) {return 'e';}
		if (scancode_high == 0x8b) {return 't';}
		if (scancode_high == 0xcf) {return 'u';}
		if (scancode_high == 0xd1) {return 'o';}
		if (scancode_high == 0x87) {return 'a';}
		if (scancode_high == 0x8d) {return 'g';}
		return  0x00;
	}
	
	return 0x00;
}


char ConsoleGetChar(void) {
    uint8_t scanCodeLow  = 0;
    uint8_t scanCodeHigh = 0;
    
    bus_read_io(&bus, 0x90000, &scanCodeLow);
    bus_read_io(&bus, 0x90001, &scanCodeHigh);
    
    return kbDecodeScanCode(scanCodeLow, scanCodeHigh);
}















/*




#include <avr/io.h>

extern "C" {
#include <kernel/kernel.h>
#include <kernel/array.hpp>
}

array::array() : mSize(0), mCapacity(0), mAddress(0), mDummyIndex(-1), mDummy(0) {}

array::array(uint32_t size) : mSize(size), mCapacity(size), mAddress(new(size)), mDummyIndex(-1), mDummy(0) {}

array::array(const array& arr) : mSize(arr.mSize), mCapacity(arr.mCapacity), mAddress(new(arr.mCapacity)), mDummyIndex(-1), mDummy(0) {
    uint8_t buffer[mSize];
    VirtualRead(arr.mAddress, buffer, mSize);
    VirtualWrite(mAddress, buffer, mSize);
}

array::~array() {
    if (mAddress > 0) 
        delete(mAddress);
}

uint8_t& array::operator[](unsigned int const i) {
    if (mDummyIndex != i) {
        updateBuffer(mDummyIndex, mDummy);
    }
    mDummyIndex = i;
    mDummy = readBuffer(i);
    return mDummy;
}

const uint8_t& array::operator[](unsigned int const i) const {
    mDummy = readBuffer(i);
    return mDummy;
}



void array::updateBuffer(uint32_t index, uint8_t value) {
    uint8_t buffer[mSize];
    VirtualRead(mAddress, buffer, mSize);
    buffer[index] = value;
    VirtualWrite(mAddress, buffer, mSize);
}

uint8_t array::readBuffer(uint32_t index) const {
    uint8_t buffer[mSize];
    VirtualRead(mAddress, buffer, mSize);
    return buffer[index];
}

*/

