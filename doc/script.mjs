import {createApp} from './node_modules/vue/dist/vue.esm-browser.prod.js';

// Components

const Slider = {
    template: `<input type="range">1.0`,
};

// Init Vue

const app = createApp({});
app.component('Slider', Slider);
