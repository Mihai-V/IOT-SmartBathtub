export const formatFloat = (number, decimals = 2) => {
    return Number.parseFloat(number).toFixed(decimals);
}

export const copyObject = (obj) => {
    return JSON.parse(JSON.stringify(obj));
}

export const bufferToString = (buf) => {
    return String.fromCharCode.apply(null, new Uint16Array(buf));
}