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
                            match: /<script src='scripts\/app.js'\/>/,
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

    // Custom task to update web_interface.h from min2.html
    grunt.registerTask('update-arduino', 'Update web_interface.h from min2.html', function() {
        const fs = require('fs');
        const path = require('path');
        
        // Read the generated min2.html
        const min2Path = path.join(__dirname, 'min2.html');
        if (!fs.existsSync(min2Path)) {
            grunt.fail.fatal('min2.html not found. Run grunt build first.');
        }
        
        let htmlContent = fs.readFileSync(min2Path, 'utf8');
        
        // Generate web_interface.h content
        const header = `#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

// HTML Web Interface for WordClock
// Stored in program memory (PROGMEM) to save RAM on ESP8285
// AUTO-GENERATED - Do not edit manually!
// This file is automatically updated when running: grunt in Frontend directory

const char web_interface[] PROGMEM = R"====(${htmlContent})====";

#endif // WEB_INTERFACE_H
`;

        // Write to web_interface.h
        const outputPath = path.join(__dirname, '../Backend/Arduino_Berndeutsch_121/web_interface.h');
        fs.writeFileSync(outputPath, header, 'utf8');
        
        grunt.log.ok('✅ web_interface.h updated successfully!');
        grunt.log.ok('   Location: ' + outputPath);
        grunt.log.ok('   Size: ' + (header.length / 1024).toFixed(2) + ' KB');
    });

    grunt.registerTask('default', [
        'terser',
        'cssmin',
        'htmlmin',
        'replace',
        'update-arduino'
    ]);
};
