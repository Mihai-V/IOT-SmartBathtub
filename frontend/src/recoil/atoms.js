import { atom } from 'recoil';

export const appState = atom({
    key: 'appState',
    default: {
        shower: {
            isOn: false,
            temperature: null,
            debit: null
        },
        bath: {
            isOn: false,
            temperature: null,
            debit: null
        },
        currentVolume: 0
    }
})