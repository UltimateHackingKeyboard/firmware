import {createApp} from './node_modules/vue/dist/vue.esm-browser.prod.js';

// Components

let currentCommand = '';
const variablesToWidgets = {};

function initWidgetValue(name, value) {
    variablesToWidgets[name]?.initValue(value);
}

function setVariable(name, value) {
    const regexVarName = name.replace(/\\./g, '\\.');
    const regex = new RegExp(`(set +${regexVarName} +)\\S+( *#?)`);
    currentCommand = currentCommand.replace(regex, `$1${value}$2`);
    const message = {command: currentCommand};
    window.parent.postMessage(message);
}

const Checkbox = {
    template: `<input type="checkbox" ref="input" @input="updateValue(false)">`,
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
        this.updateValue();
        variablesToWidgets[this.name] = this;
    },
    methods: {
        initValue(value) {
            this.$refs.input.value = value;
            this.updateValue(true);
        },
        updateValue(isInit=false) {
            this.value = this.$refs.input.checked ? 1 : 0;
            if (!isInit) {
                setVariable(this.name, this.value);
            }
        },
    },
};

const Slider = {
    template: `<input type="range" ref="range" @input="updateValue(false)">{{value}}`,
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
        this.updateValue();
        variablesToWidgets[this.name] = this;
    },
    methods: {
        initValue(value) {
            this.$refs.range.value = value;
            this.updateValue(true);
        },
        updateValue(isInit=false) {
            this.value = this.$refs.range.value;
            if (!isInit) {
                setVariable(this.name, this.value);
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
