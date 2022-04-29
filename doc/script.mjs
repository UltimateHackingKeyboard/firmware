import {createApp} from './node_modules/vue/dist/vue.esm-browser.prod.js';

// Components

const variablesToWidgets = {};

function initWidgetValue(name, value) {
    variablesToWidgets[name]?.initValue(value);
}

function setVariable(name, value) {
    console.log(`set ${name} ${value}`);
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

const app = createApp({});
app.component('Slider', Slider);
app.mount('body');
