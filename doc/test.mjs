import {createApp} from './node_modules/vue/dist/vue.esm-browser.prod.js';
import Split from './node_modules/split.js/dist/split.es.js';

let currentTarget;

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
        window.addEventListener('message', function(event) {
            currentTarget.value = event.data.command;
        });
    },
    methods: {
        change(event) {
            if (event !== 'module') {
                currentTarget = event.target;
                this.command = currentTarget.value;
            }
            const message = {
                version: '1.0.0',
                method: 'change',
                modules: {
                    keycluster: this.keycluster,
                    trackball: this.trackball,
                    trackpoint: this.trackpoint,
                    touchpad: this.touchpad,
                },
                command: this.command,
            };
            this.$refs.iframe.contentWindow.postMessage(message, '*');
        },
    },
});

app.mount('body');
