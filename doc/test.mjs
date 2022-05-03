import {createApp} from './node_modules/vue/dist/vue.esm-browser.prod.js';
import Split from './node_modules/split.js/dist/split.es.js';

const app = createApp({
    data() {
        return {
            count: 0,
        };
    },
    mounted() {
        Split(['#left', '#right'], {
            sizes: [50, 50],
        });
    },
    methods: {
        change(target) {
            console.log(target.target);
        },
    },
});

app.mount('body');
