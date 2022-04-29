import {createApp} from './node_modules/vue/dist/vue.esm-browser.prod.js';

// Components

function setVariable(name, value) {
    console.log(`set ${name} ${value}`);
}

const Slider = {
    template: `<input type="range" ref="range" @input="updateValue()">{{value}}`,
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
    },
    methods: {
        updateValue() {
            this.value = this.$refs.range.value;
            setVariable(this.name, this.value);
        },
    },
};

// Init Vue

const app = createApp({});
app.component('Slider', Slider);
app.mount('body');
