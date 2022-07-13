import {createApp} from './node_modules/vue/dist/vue.esm-browser.prod.js';
//import Select2 from './node_modules/v-select2-component/dist/Select2.esm.js';

// Components

let currentCommand = '';
const widgetDict = {};

function updateWidgets() {
    for (const widgetName in widgetDict) {
        const widget = widgetDict[widgetName];
        widget.initWidget();
    }
}

function getVariable(name) {
    const regexVarName = name.replace(/\\./g, '\\.');
    const regex = new RegExp(`set +${regexVarName} +(\\S+) *#?`);
    const matches = currentCommand.match(regex);
    const value = matches?.[1];
    return value;
}

function setVariable(name, value) {
    const regexVarName = name.replace(/\\./g, '\\.');
    const regex = new RegExp(`(set +${regexVarName} +)\\S+( *#?)`);
    if (regex.test(currentCommand)) {
        currentCommand = currentCommand.replace(regex, `$1${value}$2`);
    } else {
        currentCommand += (currentCommand.endsWith('\n') || currentCommand.trim() === '' ? '' : '\n') + `set ${regexVarName} ${value}\n`;
    }
    const message = {
        action: 'doc-message-set-macro',
        command: currentCommand,
    };

    window.parent.postMessage(message, '*');
}

const Checkbox = {
    template: `<input type="checkbox" ref="input" @input="updateValue">`,
    props: {
        name: String,
        default: Number,
    },
    data() {
        return {
            value: this.default,
        };
    },
    mounted() {
        this.updateValue(true);
        widgetDict[this.name] = this;
    },
    methods: {
        initWidget() {
            const value = getVariable(this.name) ?? this.default;
            this.value = this.$refs.input.value = value;
        },
        updateValue(isInit) {
            if (isInit === true) {
                this.$refs.input.checked = this.default === '1' ? 'checked' : 0;
            }
            this.value = +this.$refs.input.checked ? 1 : 0;
            if (isInit !== true) {
                setVariable(this.name, this.value, isInit);
            }
        },
    },
};

const Dropdown = {
    template: `<select ref="input" @input="updateValue" class="form-select"><option v-for="option in selectOptions" :value="option.value">{{option.label}}</option></select>`,
    props: {
        name: String,
        options: String,
        default: String,
    },
    data() {
        return {
            value: this.default,
            selectOptions: [],
        };
    },
    async mounted() {
        const allOptions = {
            modifierTriggers: [
                {label:'Both', value:'both'},
                {label:'Left', value:'left'},
                {label:'Right', value:'right'},
            ],
            navigationModes: [
                {label:'Cursor', value:'cursor'},
                {label:'Scroll', value:'scroll'},
                {label:'Caret', value:'caret'},
                {label:'Media', value:'media'},
                {label:'Zoom', value:'zoom'},
                {label:'Zoom PC', value:'zoomPc'},
                {label:'Zoom Mac', value:'zoomMac'},
                {label:'None', value:'none'},
            ],
            touchpadNavigationModes: [
                {label:'Zoom', value:'zoom'},
                {label:'Zoom PC', value:'zoomPc'},
                {label:'Zoom Mac', value:'zoomMac'},
                {label:'Media', value:'media'},
                {label:'None', value:'none'},
            ],
        }
        this.selectOptions = allOptions[this.options];
        await this.$nextTick();
        this.updateValue(true);
        widgetDict[this.name] = this;
    },
    methods: {
        initWidget() {
            const value = getVariable(this.name) ?? this.default;
            this.value = this.$refs.input.value = value;
        },
        updateValue(isInit) {
            if (isInit === true) {
                this.$refs.input.value = this.default;
            }

            this.value = this.$refs.input.value;
            if (isInit !== true) {
                setVariable(this.name, this.value, isInit);
            }
        },
    },
};

const Slider = {
    template: `<input type="range" ref="input" @input="updateValue" :min="min" :max="max" :step="step">{{value}}`,
    props: {
        name: String,
        min: Number,
        max: Number,
        step: Number,
        default: Number,
    },
    data() {
        return {
            value: this.default,
        };
    },
    mounted() {
        this.updateValue(true);
        widgetDict[this.name] = this;
    },
    methods: {
        initWidget() {
            const value = getVariable(this.name) ?? this.default;
            this.value = this.$refs.input.value = value;
        },
        updateValue(isInit) {
            if (isInit === true) {
                this.$refs.input.value = this.default;
            }
            this.value = this.$refs.input.value;
            if (isInit !== true) {
                setVariable(this.name, this.value, isInit);
            }
        },
    },
};

const Select2 = {
    props: {
        options: Object,
        modelValue: String,
    },
    emits: [
        'update:modelValue',
    ],
    template: '<select><slot></slot></select>',
    mounted() {
        const vm = this;
        $(this.$el)
            .select2({data: this.options})
            .val(this.value)
            .trigger('update:modelValue')
            .on('change', function() {
                vm.$emit('update:modelValue', this.value);
            });
    },
    watch: {
        value(value) {
            $(this.$el).val(value).trigger('change');
        },
        options(options) {
            $(this.$el).empty().select2({data: options});
        }
    },
    destroyed() {
        $(this.$el).off().select2('destroy');
    }
};

// Init Vue

const app = createApp({
    data() {
        return {
            modules: [],
            moduleStrings: {
                2: 'keycluster',
                3: 'trackball',
                4: 'trackpoint',
                5: 'touchpad',
            },
            moduleDescriptions: {
                2: 'Key cluster',
                3: 'Trackball',
                4: 'Trackpoint',
                5: 'Touchpad',
            },
            moduleSpeedProps: [
                {name:'baseSpeed', desc:'Base speed', min:0, max:10, step:0.1,
                    perModuleDefaults: {3:0.5, 4:0.0, 5:0.5},
                },
                {name:'speed', desc:'Speed', min:0, max:10, step:0.1,
                    perModuleDefaults: {3:0.5, 4:1.0, 5:0.7},
                },
                {name:'xceleration', desc:'Acceleration speed', min:0, max:1, step:0.1,
                    perModuleDefaults: {3:1.0, 4:0.0, 5:1.0},
                },
                {name:'caretSpeedDivisor', desc:'Caret speed divisor', min:0, max:100, step:1,
                    perModuleDefaults: {3:16.0, 4:16.0, 5:16.0},
                },
                {name:'scrollSpeedDivisor', desc:'Scroll speed divisor', min:0, max:100, step:1,
                    perModuleDefaults: {3:8.0, 4:8.0, 5:8.0},
                },
            ],
            axisLockSkewDefaults: {2:0.5, 3:0.5, 4:0.5, 5:0.5},
            axisLockFirstTickSkewDefaults: {2:0.5, 3:2.0, 4:2.0, 5:2.0},
            layers: [
                'Base',
                'Mod',
                'Mouse',
                'Fn',
                'Fn2',
                'Fn3',
                'Fn4',
                'Fn5',
                'Shift',
                'Ctrl',
                'Alt',
                'Super',
            ],
            modifiers: [
                'Shift',
                'Ctrl',
                'Alt',
                'Super',
            ],
            modifierTriggers: [
                'Both',
                'Left',
                'Right',
            ],
            keySpeedProps: [
                {
                    name:'initialSpeed',
                    desc:'Initial speed',
                    sliderProps: {
                        move: {min:25, default:100, max:6375, step:25},
                        scroll: {min:1, default:20, max:255, step:1},
                    },
                },
                {
                    name:'baseSpeed',
                    desc:'Base speed',
                    sliderProps: {
                        move: {min:25, default:800, max:6375, step:25},
                        scroll: {min:1, default:20, max:255, step:1},
                    },
                },
                {
                    name:'acceleration',
                    desc:'Acceleration',
                    sliderProps: {
                        move: {min:25, default:1700, max:6375, step:25},
                        scroll: {min:1, default:20, max:255, step:1},
                    },
                },
                {
                    name:'deceleratedSpeed',
                    desc:'Decelerated speed',
                    sliderProps: {
                        move: {min:25, default:200, max:6375, step:25},
                        scroll: {min:1, default:10, max:255, step:1},
                    },
                },
                {
                    name:'acceleratedSpeed',
                    desc:'Accelerated speed',
                    sliderProps: {
                        move: {min:25, default:1600, max:6375, step:25},
                        scroll: {min:1, default:50, max:255, step:1},
                    },
                },
                {
                    name:'axisSkew',
                    desc:'Axis skew',
                    sliderProps: {
                        move: {min:0.5, default:1, max:2, step:0.1},
                        scroll: {min:0.5, default:1, max:2, step:0.1},
                    },
                },
            ],
            scancodes: [
                'enter',
                'escape',
                'backspace',
                'tab',
                'space',
                'minusAndUnderscore',
                'equalAndPlus',
                'openingBracketAndOpeningBrace',
                'closingBracketAndClosingBrace',
                'backslashAndPipeIso',
                'backslashAndPipe',
                'nonUsHashmarkAndTilde',
                'semicolonAndColon',
                'apostropheAndQuote',
                'graveAccentAndTilde',
                'commaAndLessThanSign',
                'dotAndGreaterThanSign',
                'slashAndQuestionMark',
                'capsLock',
                'printScreen',
                'scrollLock',
                'pause',
                'insert',
                'home',
                'pageUp',
                'delete',
                'end',
                'pageDown',
                'numLock',
                'nonUsBackslashAndPipe',
                'application',
                'power',
                'keypadEqualSign',
                'execute',
                'help',
                'menu',
                'select',
                'stop',
                'again',
                'undo',
                'cut',
                'copy',
                'paste',
                'find',
                'mute',
                'volumeUp',
                'volumeDown',
                'lockingCapsLock',
                'lockingNumLock',
                'lockingScrollLock',
                'keypadComma',
                'keypadEqualSignAs400',
                'international1',
                'international2',
                'international3',
                'international4',
                'international5',
                'international6',
                'international7',
                'international8',
                'international9',
                'lang1',
                'lang2',
                'lang3',
                'lang4',
                'lang5',
                'lang6',
                'lang7',
                'lang8',
                'lang9',
                'alternateErase',
                'sysreq',
                'cancel',
                'clear',
                'prior',
                'return',
                'separator',
                'out',
                'oper',
                'clearAndAgain',
                'crselAndProps',
                'exsel',
                'keypad00',
                'keypad000',
                'thousandsSeparator',
                'decimalSeparator',
                'currencyUnit',
                'currencySubUnit',
                'keypadOpeningParenthesis',
                'keypadClosingParenthesis',
                'keypadOpeningBrace',
                'keypadClosingBrace',
                'keypadTab',
                'keypadBackspace',
                'keypadA',
                'keypadB',
                'keypadC',
                'keypadD',
                'keypadE',
                'keypadF',
                'keypadXor',
                'keypadCaret',
                'keypadPercentage',
                'keypadLessThanSign',
                'keypadGreaterThanSign',
                'keypadAmp',
                'keypadAmpAmp',
                'keypadPipe',
                'keypadPipePipe',
                'keypadColon',
                'keypadHashmark',
                'keypadSpace',
                'keypadAt',
                'keypadExclamationSign',
                'keypadMemoryStore',
                'keypadMemoryRecall',
                'keypadMemoryClear',
                'keypadMemoryAdd',
                'keypadMemorySubtract',
                'keypadMemoryMultiply',
                'keypadMemoryDivide',
                'keypadPlusAndMinus',
                'keypadClear',
                'keypadClearEntry',
                'keypadBinary',
                'keypadOctal',
                'keypadDecimal',
                'keypadHexadecimal',
                'keypadSlash',
                'keypadAsterisk',
                'keypadMinus',
                'keypadPlus',
                'keypadEnter',
                'keypad1AndEnd',
                'keypad2AndDownArrow',
                'keypad3AndPageDown',
                'keypad4AndLeftArrow',
                'keypad5',
                'keypad6AndRightArrow',
                'keypad7AndHome',
                'keypad8AndUpArrow',
                'keypad9AndPageUp',
                'keypad0AndInsert',
                'keypadDotAndDelete',
                'leftControl',
                'leftShift',
                'leftAlt',
                'leftGui',
                'rightControl',
                'rightShift',
                'rightAlt',
                'rightGui',
                'up',
                'down',
                'left',
                'right',
                'upArrow',
                'downArrow',
                'leftArrow',
                'rightArrow',
                'np0',
                'np1',
                'np2',
                'np3',
                'np4',
                'np5',
                'np6',
                'np7',
                'np8',
                'np9',
                'f1',
                'f2',
                'f3',
                'f4',
                'f5',
                'f6',
                'f7',
                'f8',
                'f9',
                'f10',
                'f11',
                'f12',
                'f13',
                'f14',
                'f15',
                'f16',
                'f17',
                'f18',
                'f19',
                'f20',
                'f21',
                'f22',
                'f23',
                'f24',
                'mediaVolumeMute',
                'mediaVolumeUp',
                'mediaVolumeDown',
                'mediaRecord',
                'mediaFastForward',
                'mediaRewind',
                'mediaNext',
                'mediaPrevious',
                'mediaStop',
                'mediaPlayPause',
                'mediaPause',
                'systemPowerDown',
                'systemSleep',
                'systemWakeUp',
                'mouseBtnLeft',
                'mouseBtnRight',
                'mouseBtnMiddle',
                'mouseBtn4',
                'mouseBtn5',
                'mouseBtn6',
                'mouseBtn7',
                'mouseBtn8',
            ],
            selected: 'enter',
            options: [{ id: 1, text: "Hello" }, { id: 2, text: "World" }],
        }
    },
    created() {
        const self = this;
        window.addEventListener('message', function(event) {
            switch (event.data.action) {
                case 'agent-message-editor-got-focus': {
                    const data = event.data;
                    currentCommand = data.command;
                    self.modules = data.modules;
                    updateWidgets();
                    break;
                }

                case 'agent-message-editor-lost-focus': {
                    const data = event.data;
                    currentCommand = '';
                    self.modules = data.modules;
                    updateWidgets();
                    break;
                }
            }
        });
        window.parent.postMessage({action: 'doc-message-inited'}, '*');
    },
    methods: {
        getNavigationMode(module, layer) {
            if (module === 2) {
                return {Base:'scroll', Mod:'cursor', Fn:'caret'}[layer] ?? 'cursor';
            }
            return {Base:'cursor', Mod:'scroll', Fn:'caret'}[layer] ?? 'cursor';
        },
    },
    computed: {
        rightModules() {
            return this.modules.filter(module => module !== 2);
        },
        isTouchpadAttached() {
            return this.modules.includes(5);
        }
    },
    myChangeEvent(val){
        console.log(val);
    },
    mySelectEvent({id, text}){
        console.log({id, text})
    },
});

app.component('Checkbox', Checkbox);
app.component('Dropdown', Dropdown);
app.component('Slider', Slider);
app.component('Select2', Select2);

app.mount('body');
