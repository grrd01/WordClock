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
                    'min.html': 'index.html'
                }
            }
        },
        replace: {
            dist: {
                options: {
                    patterns: [
                        {
                            match: /<script src='scripts\/app.js'><\/script>/,
                            replacement: function() {
                                return "\n<script>" + grunt.file.read('scripts/min.js') + "</script>";
                            }
                        }
                    ]
                },
                files: {
                    'min.html': 'min.html'
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
                    'min.html': 'min.html'
                }
            },
            dist3: {
                options: {
                    patterns: [
                        {
                            match: /\\'/g,
                            replacement: '\\\"'
                        }
                    ]
                },
                files: {
                    'min2.html': 'min.html'
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
