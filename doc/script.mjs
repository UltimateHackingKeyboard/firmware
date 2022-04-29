import {createApp} from './node_modules/vue/dist/vue.esm-browser.prod.js';

// Components

const Slider = {
    template: `<input type="range" ref="range" @input="updateValue()">{{ value }}`,
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
            console.log(this.value);
        },

    },
};

// Init Vue

const app = createApp({});
app.component('Slider', Slider);
app.mount('body');
