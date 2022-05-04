import {createApp} from './node_modules/vue/dist/vue.esm-browser.prod.js';

// Components

let currentCommand = '';
const variablesToWidgets = {};

function initWidgetValue(name, value) {
    variablesToWidgets[name]?.initValue(value);
}

function setVariable(name, value) {
    const regexVarName = name.replace(/\\./g, '.');
    const regex = new RegExp(`(set +${regexVarName} +)\\S+( *#?)`);
    const newCommand = currentCommand.replace(regex, `$1${value}$2`);
    console.log(`set ${name} ${value}`);
    const message = {command: newCommand};
    console.log('child send:', message);
    window.parent.postMessage(message);
}

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
    created() {
        window.addEventListener('message', function(event) {
            console.log('child receive:', event.data);
            currentCommand = event.data.command;
        });
    },
});
app.component('Slider', Slider);
app.mount('body');
