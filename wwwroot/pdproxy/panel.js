// PDP 11/70 Panel
//
// Based on the Javascript PDP 11/70 Emulator v4.0 written by Paul Nankervis
//
// This code may be used freely provided the original author name (Paul Nankervis)
// is acknowledged in any modified source code
//
// This code reads PDP 11/70 panel state from a WebSocket and updates the virtual
// panel it shows accordingly.
//

var panel = {
    switchRegister: 0, // console switch register
    addressId: [], // DOM id's for addressLights
    addressLights: 0x3fffff, // current state of addressLights (a0-a21)
    autoIncr: 0, // panel auto increment
    displayId: [], // DOM id's for displayLights
    displayLights: 0xffff, // current state of displayLights (d0-d15)
    halt: 0, // halt switch position
    lampTest: 0, // lamp switch test position
    powerSwitch: 0, // -1 off, 0 run, 1 locked
    rotary0: 0, // rotary switch 0 position
    rotary1: 0, // rotary switch 1 position
    statusId: [], //  DOM id's for statusLights
    statusLights: 0x3ffffff, // current state of statusLights (s0-s25)
    old_state: null, // previous state of panel
    state: null, // current state of panel, as received from WebSocket
    step: 0 // S Inst / S Bus switch position
};

function moveSwitch(id, position) { // -1 up  0 centre   1 down  - will move 5/16 units
    "use strict";
    id.style.borderTopWidth = 'calc(var(--unitHeight) * ' + (8 + 4 * position) + ')';
    id.style.borderBottomWidth = 'calc(var(--unitHeight) * ' + (7 - 4 * position) + ')';
}

function setSwitch(id, weight) {
    "use strict";
    var mask = 1 << weight;
    panel.switchRegister ^= mask;
    moveSwitch(id, (panel.switchRegister & mask) ? -1 : 0);
}

function toggleSwitch(id) {
    "use strict";
    moveSwitch(id, 1);
    setTimeout(function() {
        moveSwitch(id, 0);
    }, 350);
}

function initPanel(idArray, idName, idCount) {
    "use strict";
    let id, elementId, initVal = 0;
    for (id = 0; id < idCount; id++) {
        if ((elementId = document.getElementById(idName + id))) {
            idArray[id] = elementId.style;
        } else {
            idArray[id] = {}; // If element not present make a dummy
        }
        initVal = initVal * 2 + 1;
    }
    return initVal;
}

function setPanelState(newState) {
    "use strict";
    panel.old_state = panel.state; // Save old state
    panel.state = newState; // Set new state
}

// There are three groups of lights (LEDs/Globes):-
//  addressLights (a0-a21) which show the current address in panel state
//  displayLights (d0-d15) shows current data in panel state
//  statusLights (s0-s25) all else from MMU status, CPU mode, Bus status,
//  parity, and position of rotary switches
//
// The updateLights() function should be executed every time any state changes that
// may be relevant for the panel lights, to calculate the three light bit mask values
// and then set the appropriate light visibility to either hidden or visible.
//
// statusLights:         25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
//                      |  rotary1  |       rotary0         | PAR |PE AE Rn Pa Ma Us Su Ke Da 16 18 22

function updateLights() {
    "use strict";
    // default all address and data lights to off, some status lights don't depend
    // on the panel status we receive via the WebSocket
    let addressLights = 0,
        displayLights = 0,
        statusLights = (0x400000 << panel.rotary1)
            | (0x4000 << panel.rotary0)
            | 0x3000; // Parity high/low, always on

    function updatePanel(oldMask, newMask, idArray) { // Update lights to match newMask
        let mask = newMask ^ oldMask; // difference mask
        for (let id = 0; mask; id++, mask >>>= 1) {
            while (!(mask & 1)) {
                id++;
                mask >>>= 1;
            }
            if (newMask & (1 << id)) {
                idArray[id].visibility = 'visible';
                idArray[id].opacity = '1';
                idArray[id].transition = '';
            } else {
                idArray[id].transition = 'opacity 150ms ease-out';
                idArray[id].opacity = '0';
            }
        }
    }

    if (panel.lampTest) {
        addressLights = 0x3fffff;
        displayLights = 0xffff;
        statusLights = 0x3ffffff;
    } else if (panel.state) {
        addressLights = panel.state.address & 0x3fffff; // Mask to 22 bits
        displayLights = panel.state.data & 0xffff; // Mask to 16 bits

        // We assume that the data space was accessed if the data field has changed
        let data_space = !panel.old_state || (displayLights ^ (panel.old_state.data & 0xffff)) !== 0;

        // PE AE Rn Pa Ma Us Su Ke Da 16 18 22
        statusLights |=
              (panel.state.parity_error ? 0x800 : 0) // Parity error
            | (panel.state.address_error ? 0x400 : 0) // Address error
            | (0x200) // Run - on, otherwise we wouldn't be here. Likewise, Pause is off
            | (0x80) // Master - on, which means we assume DMA I/O is not a thing
            | (panel.state.user ? 0x40 : 0) // User
            | (panel.state.super ? 0x20 : 0) // Super
            | (panel.state.kernel ? 0x10 : 0) // Kernel
            | (data_space ? 0x8 : 0) // Data space access
            | (panel.state.addr16 ? 0x4 : 0) // Address mode 16
            | (panel.state.addr18 ? 0x2 : 0) // Address mode 18
            | (panel.state.addr22 ? 0x1 : 0); // Address mode 22
    }

    if (addressLights !== panel.addressLights) {
        updatePanel(panel.addressLights, addressLights, panel.addressId);
        panel.addressLights = addressLights;
    }
    if (displayLights !== panel.displayLights) {
        updatePanel(panel.displayLights, displayLights, panel.displayId);
        panel.displayLights = displayLights;
    }
    if (statusLights !== panel.statusLights) {
        updatePanel(panel.statusLights, statusLights, panel.statusId);
        panel.statusLights = statusLights;
    }
}

function initialize() {
    // One off functions to find light objects
    panel.addressLights = initPanel(panel.addressId, "a", 22);
    panel.displayLights = initPanel(panel.displayId, "d", 16);
    panel.statusLights = initPanel(panel.statusId, "s", 26);

    updateLights(); // Initial update of lights
}