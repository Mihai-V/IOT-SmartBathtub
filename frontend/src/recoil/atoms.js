import { atom } from 'recoil';

export const appState = atom({
    key: 'appState',
    default: {
        shower: {
            isOn: false,
            temperature: 20,
            debit: null
        },
        bath: {
            isOn: false,
            temperature: 20,
            debit: null
        },
        currentVolume: 0,
        badWaterQuality: false
    }
})