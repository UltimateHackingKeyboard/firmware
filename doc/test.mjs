import {createApp} from './node_modules/vue/dist/vue.esm-browser.prod.js';
import Split from './node_modules/split.js/dist/split.es.js';

const app = createApp({
    data() {
        return {
            keycluster: false,
            trackball: false,
            trackpoint: false,
            touchpad: false,
            command: '',
        };
    },
    mounted() {
        Split(['#left', '#right'], {
            sizes: [50, 50],
        });
    },
    methods: {
        change(event) {
            if (event !== 'module') {
                this.command = event.target.value;
            }
            const message = {
                modules: {
                    keycluster: this.keycluster,
                    trackball: this.trackball,
                    trackpoint: this.trackpoint,
                    touchpad: this.touchpad,
                },
                command: this.command,
            };
            console.log(message);
            this.$refs.iframe.contentWindow.postMessage(message, "*");
        },
    },
});

app.mount('body');
