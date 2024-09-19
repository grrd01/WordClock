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
        }
    });

    grunt.loadNpmTasks('grunt-terser');
    grunt.loadNpmTasks('grunt-contrib-cssmin');
    grunt.loadNpmTasks('grunt-contrib-htmlmin');

    grunt.registerTask('default', [
        'terser',
        'cssmin',
        'htmlmin'
    ]);
};
