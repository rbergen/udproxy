"use strict";

const startupRegisterValue = 'FFFFFFFFFFFFFFFF';
const defaultRegisterValue = '0000000000000000'; // Default value for registers

var panel = {
    registers: [
        'rax',
        'rbx',
        'rcx',
        'rdx',
        'rsi',
        'rdi',
        'rbp',
        'rsp',
        'rip',
        'rflags'],
    old_state: null, // previous state of panel
    state: null, // current state of panel, as received from WebSocket
};

function updateRegister(name, oldValue, newValue) {
    function update32Bits(bitDivs, offset, oldValue, newValue) {
        const oldNumericValue = parseInt(oldValue, 16);
        const newNumericValue = parseInt(newValue, 16);
        let diff = newNumericValue ^ oldNumericValue; // Calculate difference

        for (let bitIndex = 0; diff; bitIndex++, diff >>>= 1) {
            while (!(diff & 1)) {
                bitIndex++;
                diff >>>= 1;
            }
            bitDivs[offset + bitIndex].style.visibility = (newNumericValue & (1 << bitIndex)) ? 'visible' : 'hidden';
        }
    }

    const bits_element = document.getElementById(name + "_bits");
    if (bits_element) {
        // The first bit div in the document represents the most significant bit, so we reverse the order
        const bitDivs = [...bits_element.querySelectorAll(':scope > div.bit')].reverse();
        update32Bits(bitDivs, 0, oldValue.substring(8, 16), newValue.substring(8, 16));
        update32Bits(bitDivs, 32, oldValue.substring(0, 8), newValue.substring(0, 8));
    }

    const value_element = document.getElementById(name + "_value");
    value_element && (value_element.textContent = newValue); // Update displayed value
}

function updatePanelState(newState) {
    panel.old_state = panel.state; // Save old state
    panel.state = newState; // Set new state

    for (const register of Object.keys(panel.state)) {
        const oldValue = panel.old_state ? (panel.old_state[register] || defaultRegisterValue) : defaultRegisterValue;
        const newValue = panel.state[register];
        if (newValue !== oldValue)
            updateRegister(register, oldValue, newValue);
    }
}

function initialize() {
    // Initialize all registers with default values
    for (const name of panel.registers)
        updateRegister(name, startupRegisterValue, defaultRegisterValue);
}