/**
 * grrd's WordClock
 * Copyright (c) 2024 Gerard Tyedmers, grrd@gmx.net
 * @license MPL-2.0
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

(function () {
    "use strict";

    let date;
    let hour;
    let minute;
    let power = 1;
    let darkMode = 1;
    let ghost = 1;
    let rainbow = 0;
    let rainbowRed = 255;
    let rainbowGreen = 0;
    let rainbowBlue = 0;
    let score = 0;
    let highscore = 0;
    let mastermindColor = "1";
    let mastermindWeiss = 0;
    let mastermindGrau = 0;
    let mastermindTry = 0;
    function $ (id) {
        return document.getElementById(id);
    }
    function $$ (key) {
        return localStorage.getItem(key);
    }
    function $$$ (key, value) {
        return localStorage.setItem(key, value);
    }
    const clock = $("clock");
    const pageClock = $("pageClock");
    const pageSettings = $("pageSettings");
    const pageSnake = $("pageSnake");
    const color = $("color");
    const speed = $("speed");

    /**
     * Set the current time
     */
    function setTime () {
        date = new Date();
        if (darkMode && (date.getHours() >=22 || date.getHours() < 7)) {
            document.getElementsByTagName("body")[0].classList.add("dark");
        } else {
            document.getElementsByTagName("body")[0].classList.remove("dark");
        }
        if (minute === date.getMinutes()) {
            return;
        }
        minute = date.getMinutes();
        clock.classList.remove(...clock.classList);
        if (power === 0) {
            return;
        }
        if (minute >= 55) {
            clock.classList.add("M5", "MV");
        } else if (minute >= 50) {
            clock.classList.add("M10", "MV");
        } else if (minute >= 45) {
            clock.classList.add("M15", "MV");
        } else if (minute >= 40) {
            clock.classList.add("M20", "MV");
        } else if (minute >= 35) {
            clock.classList.add("M5", "MA", "M30");
        } else if (minute >= 30) {
            clock.classList.add("M30");
        } else if (minute >= 25) {
            clock.classList.add("M5", "MV", "M30");
        } else if (minute >= 20) {
            clock.classList.add("M20", "MA");
        } else if (minute >= 15) {
            clock.classList.add("M15", "MA");
        } else if (minute >= 10) {
            clock.classList.add("M10", "MA");
        } else if (minute >= 5) {
            clock.classList.add("M5", "MA");
        }
        hour = date.getHours();
        if (minute >= 25) {
            hour += 1;
        }
        hour = hour % 12;
        clock.classList.add("H" + hour.toString());
        clock.classList.add("M" + (minute % 5).toString());
    }
    setInterval(setTime, 100);

    /**
     * Change the color by one step in rainbow-mode
     */
    function fChangeRainbow() {
        if (rainbow) {
            if (rainbowRed && !rainbowBlue) {
                rainbowRed -= 1;
                rainbowGreen += 1;
            } else if (rainbowGreen) {
                rainbowGreen -= 1;
                rainbowBlue += 1;
            } else {
                rainbowBlue -= 1;
                rainbowRed += 1;
            }
            fChangeColor("rgb(" + rainbowRed + ", " + rainbowGreen + ", " + rainbowBlue + ")");
            setTimeout(fChangeRainbow, speed.value / 10);
        }
    }

    /**
     * Toggle the display on/off
     */
    function fTogglePower() {
        fSetPower(1 - power);
        fUpdateParams();
    }

    /**
     * Turn the display on/off
     */
    function fSetPower(power_in) {
        power = power_in;
        if (power) {
            document.getElementsByTagName("body")[0].classList.remove("off");
        } else {
            document.getElementsByTagName("body")[0].classList.add("off");
        }
        minute = -1;
    }

    /**
     * Display the settings-page
     */
    function fShowSettings() {
        // Fix for Firefox OnKeydown
        document.activeElement.blur();
        pageClock.classList.remove("swipe-out-right");
        pageSettings.classList.remove("swipe-in-left");
        pageClock.classList.add("swipe-out");
        pageSettings.classList.add("swipe-in");
    }

    /**
     * Set all elements to selected color
     */
    function fChangeColor(color_in) {
        document.documentElement.style.setProperty('--main-color', color_in);
    }

    /**
     * Set rainbow-mode on or off
     * @param {int} rain_in : 1 = rainbow on; 0 = rainbow off
     */
    function fRainbow(rain_in) {
        if (rain_in !== rainbow) {
            $("rainbowMode").children[0].classList.toggle("hide");
            $("rainbowMode").children[1].classList.toggle("hide");
        }
        rainbow = rain_in;
        if (rainbow) {
            fChangeRainbow();
        } else {
            fChangeColor(color.value);
        }
    }

    /**
     * Set ghost on or off
     * @param {int} ghost_in : 1 = ghost on; 0 = ghost off
     */
    function fGhost(ghost_in) {
        if (ghost_in !== ghost) {
            $("ghostMode").children[0].classList.toggle("hide");
            $("ghostMode").children[1].classList.toggle("hide");
        }
        ghost = ghost_in;
    }

    /**
     * Set dark-mode on or off
     * @param {int} dark_in : 1 = dark-mode on; 0 = dark-mode off
     */
    function fDarkMode(dark_in) {
        if (dark_in !== darkMode) {
            $("darkMode").children[0].classList.toggle("hide");
            $("darkMode").children[1].classList.toggle("hide");
        }
        darkMode = dark_in;
    }

    /**
     * Return hex-color-code from decimal rgb-color
     * @param {int} r : 1 = red-value (0-255)
     * @param {int} g : 1 = green-value (0-255)
     * @param {int} b : 1 = blue-value (0-255)
     */
    function fRgb2Hex(r, g, b) {
        return "#" + (1 << 24 | r << 16 | g << 8 | b).toString(16).slice(1);
    }

    /**
     * Hide the settings-page and return to clock-page
     */
    function fHideSettings() {
        fUpdateParams();
        pageClock.classList.remove("swipe-out");
        pageSettings.classList.remove("swipe-in");
        pageSettings.classList.remove("swipe-out-right")
        pageClock.classList.add("swipe-out-right");
        pageSettings.classList.add("swipe-in-left");
    }

    /**
     * Hide the settings-page and return to clock-page
     */
    function fUpdateParams() {
        let red = parseInt(color.value.substring(1,3), 16);
        let green = parseInt(color.value.substring(3,5), 16);
        let blue = parseInt(color.value.substring(5,7), 16);
        $$$("wc_color", color.value);
        $$$("wc_rainbow", rainbow);
        $$$("wc_darkmode", darkMode);
        $$$("wc_ghost", ghost);
        $$$("wc_speed", speed.value.toString());
        if (window.location.href.includes("192.168.") || window.location.href.includes(".local")) {
            let xhr = new XMLHttpRequest();
            xhr.open("GET", "/update_params?red=" + red + "&green=" + green + "&blue=" + blue + "&rainbow=" + rainbow + "&darkmode=" + darkMode + "&speed=" + speed.value + "&power=" + power + "&ghost=" + ghost, true);
            xhr.send();
        }
    }

    /**
     * Display the snake-page
     */
    function fShowSnake() {
        // Fix for Firefox OnKeydown
        document.activeElement.blur();
        pageSettings.classList.remove("swipe-out-right");
        pageSnake.classList.remove("swipe-in-left");
        pageSettings.classList.add("swipe-out");
        pageSnake.classList.add("swipe-in");
        fSendSnake (5);
    }

    /**
     * Send snake-control-input to word-clock
     * @param {int} dir : direction for snake to move: 1=up, 2=right, 3=down, 4=left, 5=new game, 6=quit game
     */
    function fSendSnake (dir) {
        let xhttp = new XMLHttpRequest();
        xhttp.onreadystatechange = function() {
            if (this.readyState === 4 && this.status === 200) {
                score = (parseInt(xhttp.responseText) - 3) * 10;
                if (score > highscore) {
                    highscore = score;
                    $$$("wc_score",highscore);
                }
                $("scoreSnake").innerHTML = "Score: " + score + " / High-Score : " + highscore;
            }
        };
        xhttp.open("GET", "snake?dir=" + dir, true);
        xhttp.send();
    }

    /**
     * Hide the snake-page and return to settings-page
     */
    function fHideSnake() {
        pageSettings.classList.remove("swipe-out");
        pageSnake.classList.remove("swipe-in");
        pageSettings.classList.add("swipe-out-right");
        pageSnake.classList.add("swipe-in-left");
        fSendSnake (6);
    }

    /**
     * Display the mastermind-page
     */
    function fShowMastermind() {
        // Fix for Firefox OnKeydown
        document.activeElement.blur();
        pageSettings.classList.remove("swipe-out-right");
        pageMastermind.classList.remove("swipe-in-left");
        pageSettings.classList.add("swipe-out");
        pageMastermind.classList.add("swipe-in");
        fSendMastermind (1);
    }

    /**
     * Send mastermind-control-input to word-clock
     * @param {int} action : 1=new game, 2=quit game, null=send try
     */
    function fSendMastermind (action) {
        let urlparams = "";
        if (action === 1) {
            urlparams = "mastermind?c4=0";
            fClearMastermind();
            fMastermindMessage();
        } else if (action === 2) {
            urlparams = "mastermind?c4=7"
            fClearMastermind();
        } else {
            console.log(document.querySelectorAll("[data-num='1'], [data-num='2'], [data-num='3'], [data-num='4'], [data-num='5'], [data-num='6']").length);
            if (document.querySelectorAll("[data-num='1'], [data-num='2'], [data-num='3'], [data-num='4'], [data-num='5'], [data-num='6']").length < 14) {
                fMastermindMessage("Muesch zersch aues uswähle.");
                return;
            }
            urlparams = "mastermind?c1=" + document.getElementsByClassName("codeButton")[0].getAttribute("data-num")
                + "&c2="  + document.getElementsByClassName("codeButton")[1].getAttribute("data-num")
                + "&c3="  + document.getElementsByClassName("codeButton")[2].getAttribute("data-num")
                + "&c4="  + document.getElementsByClassName("codeButton")[3].getAttribute("data-num");
            fClearMastermind();
        }
        let xhttp = new XMLHttpRequest();
        xhttp.onreadystatechange = function() {
            if (this.readyState === 4 && this.status === 200) {
                let response = JSON.parse(xhttp.responseText);
                mastermindWeiss = response.place;
                mastermindGrau = response.color;
                mastermindTry = response.try;
                if (mastermindWeiss === 4) {
                    fMastermindMessage("Bravo! I " + mastermindTry + " Mau usegfunde.");
                } else if (mastermindTry === 11) {
                    fMastermindMessage("Schad, jetz hesch verlore.");
                } else {
                    fMastermindMessage();
                }
            }
        };
        xhttp.open("GET", urlparams, true);
        xhttp.send();
    }

    /**
     * Hide the mastermind-page and return to settings-page
     */
    function fHideMastermind() {
        pageSettings.classList.remove("swipe-out");
        pageMastermind.classList.remove("swipe-in");
        pageSettings.classList.add("swipe-out-right");
        pageMastermind.classList.add("swipe-in-left");
        fSendMastermind (2);
    }

    /**
     * Clear current mastermind code
     */
    function fClearMastermind() {
        Array.from(document.getElementsByClassName("codeButton")).forEach(function(element) {
            element.setAttribute("data-num", "");
        });
    }

    /**
     * Show messages in Mastermine
     */
    function fMastermindMessage(msg) {
        if (msg) {
            $("scoreMastermind").innerHTML = msg;
        } else {
            $("scoreMastermind").innerHTML = "<svg xmlns='http://www.w3.org/2000/svg' class='svgMsg' viewBox='0 0 70 70'> <circle cx='35' cy='35' r='25' fill='white'/></svg>&nbsp;am richtige Ort&nbsp;" +
                "<svg xmlns='http://www.w3.org/2000/svg' class='svgMsg' viewBox='0 0 70 70'> <circle cx='35' cy='35' r='25' fill='cornflowerblue'/></svg>&nbsp;di richtigi Farb";
        }
    }

    $("power").addEventListener("click", fTogglePower);
    $("settings").addEventListener("click", fShowSettings);
    $("settingsClose").addEventListener("click", fHideSettings);

    /**
     * Initialize application, add event-listeners
     */
    $("playSnake").addEventListener("click", fShowSnake);
    $("exitSnake").addEventListener("click", fHideSnake);
    $("playMastermind").addEventListener("click", fShowMastermind);
    $("exitMastermind").addEventListener("click", fHideMastermind);
    $("sendMastermind").addEventListener("click", fSendMastermind);
    Array.from(document.getElementsByClassName("snakeButton")).forEach(function(element) {
        element.addEventListener("click", function (e) {
            fSendSnake(e.target.getAttribute("data-num"));
        });
    });
    Array.from(document.getElementsByClassName("colorButton")).forEach(function(element) {
        element.addEventListener("click", function (e) {
            Array.from(document.getElementsByClassName("colorButton")).forEach(function(element) {
                element.classList.remove("g");
            });
            e.target.classList.add("g");
            mastermindColor = e.target.getAttribute("data-num");
        });
    });
    Array.from(document.getElementsByClassName("codeButton")).forEach(function(element) {
        element.addEventListener("click", function (e) {
            e.target.setAttribute("data-num",mastermindColor);
            fMastermindMessage()
        });
    });
    function fCheckKey(e) {
        let dir = 0;
        switch (e.key) {
            case "ArrowUp":
                dir = 1;
                break;
            case "ArrowRight":
                dir = 2;
                break;
            case "ArrowDown":
                dir = 3;
                break;
            case "ArrowLeft":
                dir = 4;
                break;
        }
        if (dir) {
            fSendSnake(dir);
            $("control").children[dir - 1].classList.add("g");
            setTimeout(function() {
                $("control").children[dir - 1].classList.remove("g");
            }, 200);
        }
    }
    document.onkeydown = fCheckKey;
    color.addEventListener("change", (ignore) => {
        fChangeColor(color.value);
    }, false);
    $("rainbowMode").addEventListener("click", (ignore) => {
        fRainbow(1 - rainbow);
    });
    $("ghostMode").addEventListener("click", (ignore) => {
        fGhost(1 - ghost);
    });
    $("darkMode").addEventListener("click", (ignore) => {
        fDarkMode(1 - darkMode);
    });

    /**
     * Reload last settings from local storage
     */
    if ($$("wc_color")) {
        color.value = $$("wc_color");
        fChangeColor(color.value);
    }
    if ($$("wc_rainbow")) {
        fRainbow(parseInt($$("wc_rainbow")));
    }
    if ($$("wc_ghost")) {
        fGhost(parseInt($$("wc_ghost")));
    }
    if ($$("wc_darkmode")) {
        fDarkMode(parseInt($$("wc_darkmode")));
    }
    if ($$("wc_speed")) {
        speed.value = (parseInt($$("wc_speed")));
    }
    if ($$("wc_score")) {
        highscore = $$("wc_score");
    }
    $("iphone").href = $("icon").href;

    /**
     * Load current settings from word-clock
     */
    if (window.location.href.includes("192.168.") || window.location.href.includes(".local")) {
        let xhttp = new XMLHttpRequest();
        xhttp.onreadystatechange = function() {
            if (this.readyState === 4 && this.status === 200) {
                let response = JSON.parse(xhttp.responseText);
                if (!response.rainbow) {
                    color.value = fRgb2Hex(response.red, response.green, response.blue);
                }
                fChangeColor(color.value);
                fDarkMode(response.darkmode);
                fRainbow(response.rainbow);
                fGhost(response.ghost);
                fSetPower(response.power);
                speed.value = response.speed;
            }
        };
        xhttp.open("GET", "get_params", true);
        xhttp.send();
    } else {
        $("snakeBody").classList.add("hide");
        $("mastermindBody").classList.add("hide");
    }

}());
