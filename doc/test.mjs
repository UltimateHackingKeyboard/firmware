import {createApp} from './node_modules/vue/dist/vue.esm-browser.prod.js';
import Split from './node_modules/split.js/dist/split.es.js';

let currentTarget;

const app = createApp({
    data() {
        return {
            keycluster: true,
            trackball: true,
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
            switch (event.data.action) {
                case 'doc-message-set-macro': {
                    currentTarget.value = event.data.command;
                    break;
                }
            }
        });
    },
    methods: {
        change(event) {
            if (event !== 'module') {
                currentTarget = event.target;
                this.command = currentTarget.value;
            }

            const modules = [
                {moduleId: 2, isAttached: this.keycluster},
                {moduleId: 3, isAttached: this.trackball},
                {moduleId: 4, isAttached: this.trackpoint},
                {moduleId: 5, isAttached: this.touchpad},
            ].filter(module => module.isAttached)
            .map(module => module.moduleId);

            const message = {
                version: '1.0.0',
                action: 'agent-message-editor-got-focus',
                modules,
                command: this.command,
            };

            this.$refs.iframe.contentWindow.postMessage(message, '*');
        },
    },
});

app.mount('body');
