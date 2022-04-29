import {createApp} from './node_modules/vue/dist/vue.esm-browser.prod.js';

// Components

const Slider = {
    template: `<input type="range" ref="range" @input="onInput()">1.0`,
    methods: {
        onInput() {
            console.log(this.$refs.range.value);
        },
    },
};

// Init Vue

const app = createApp({});
app.component('Slider', Slider);
app.mount('body');
