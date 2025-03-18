module.exports = function(grunt) {
    require('load-grunt-tasks')(grunt);
    // Project configuration.
    grunt.initConfig({
        pkg: grunt.file.readJSON('package.json'),
        terser: {
            one: {
                options: {
                    compress: true,
                    mangle: true,
                    format: {
                        quote_style: 1,
                        comments: false
                    }
                },
                files: {
                    'scripts/min.js': ['scripts/app.js']
                }
            },
            two: {
                options: {
                    compress: true,
                    mangle: true,
                    format: {
                        quote_style: 1,
                        comments: false
                    }
                },
                files: {
                    '../docs/sw.js': ['sw.js']
                }
            }
        },
        cssmin: {
            dist: {
                files: {
                    'styles/min.css': ['styles/app.css']
                }
            }
        },
        htmlmin: {
            dist: {
                options: {
                    removeComments: true,
                    collapseWhitespace: true,
                    quoteCharacter: "'"
                },
                files: {
                    '../docs/index.html': 'index.html'
                }
            }
        },
        replace: {
            dist: {
                options: {
                    patterns: [
                        {
                            match: /<script src='scripts\/app.js'\/>/,
                            replacement: function() {
                                return "\n<script>" + grunt.file.read('scripts/min.js') + "</script>";
                            }
                        }
                    ]
                },
                files: {
                    '../docs/index.html': '../docs/index.html'
                }
            },
            dist2: {
                options: {
                    patterns: [
                        {
                            match: /<link rel='stylesheet' href='styles\/app.css'>/,
                            replacement: function () {
                                return "\n<style>" + grunt.file.read('styles/min.css') + "</style>\n";
                            }
                        }
                    ]
                },
                files: {
                    '../docs/index.html': '../docs/index.html'
                }
            }
        }
    });

    grunt.loadNpmTasks('grunt-terser');
    grunt.loadNpmTasks('grunt-contrib-cssmin');
    grunt.loadNpmTasks('grunt-contrib-htmlmin');
    grunt.loadNpmTasks('grunt-replace');

    grunt.registerTask('default', [
        'terser',
        'cssmin',
        'htmlmin',
        'replace'
    ]);
};
