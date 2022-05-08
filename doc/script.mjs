import {createApp} from './node_modules/vue/dist/vue.esm-browser.prod.js';

// Components

let currentCommand = '';
const variablesToWidgets = {};

function initWidgetValue(name, value) {
    variablesToWidgets[name]?.initValue(value);
}

function setVariable(name, value, isInit) {
    const regexVarName = name.replace(/\\./g, '\\.');
    const regex = new RegExp(`(set +${regexVarName} +)\\S+( *#?)`);
    if (regex.test(currentCommand)) {
        currentCommand = currentCommand.replace(regex, `$1${value}$2`);
    } else {
        currentCommand += (currentCommand.endsWith('\n') ? '' : '\n') + `set ${regexVarName} ${value}\n`;
    }
    const message = {command: currentCommand};
    window.parent.postMessage(message);
}

const Checkbox = {
    template: `<input type="checkbox" ref="input" @input="updateValue">`,
    props: {
        name: String,
    },
    data() {
        return {
            value: false,
        };
    },
    mounted() {
        console.log('checkbox name', this.name)
        this.updateValue(true);
        variablesToWidgets[this.name] = this;
    },
    methods: {
        updateValue(isInit) {
            this.value = this.$refs.input.checked ? 1 : 0;
            if (isInit !== true) {
                setVariable(this.name, this.value, isInit);
            }
        },
    },
};

const Slider = {
    template: `<input type="range" ref="range" @input="updateValue">{{value}}`,
    props: {
        name: String,
    },
    data() {
        return {
            value: '',
        };
    },
    mounted() {
        console.log('slider name', this.name)
        this.updateValue(true);
        variablesToWidgets[this.name] = this;
    },
    methods: {
        updateValue(isInit) {
            this.value = this.$refs.range.value;
            if (isInit !== true) {
                setVariable(this.name, this.value, isInit);
            }
        },
    },
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
        };
    },
    created() {
        const self = this;
        window.addEventListener('message', function(event) {
            const data = event.data;
            currentCommand = data.command;
            self.modules = data.modules;
        });
    },
    computed: {
        rightModules() {
            return this.modules.filter(module => module !== 2);
        }
    }
});
app.component('Checkbox', Checkbox);
app.component('Slider', Slider);
app.mount('body');
