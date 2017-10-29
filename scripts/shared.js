require('shelljs/global');

function getBlhostCmd() {
    let blhostPath;
    switch (process.platform) {
        case 'linux':
            blhostPath = 'linux/amd64/blhost';
            break;
        case 'darwin':
            blhostPath = 'mac/blhost';
            break;
        case 'win32':
            blhostPath = 'win/blhost.exe';
            break;
        default:
            echo('Your operating system is not supported');
            exit(1);
            break;
    }

    return `../../../lib/bootloader/bin/Tools/blhost/${blhostPath} --usb 0x1d50,0x6121`;
}

function execRetry(command) {
    let firstRun = true;
    let remainingRetries = 3;
    let code;
    do {
        if (!firstRun) {
            console.log(`Retrying ${command}`)
        }
        config.fatal = !remainingRetries;
        code = exec(command).code;
        config.fatal = true;
        firstRun = false;
    } while(code && --remainingRetries);
}

const exp = {
    getBlhostCmd,
    execRetry,
}

Object.keys(exp).forEach(function (cmd) {
  global[cmd] = exp[cmd];
});
