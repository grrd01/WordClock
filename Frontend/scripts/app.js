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

    const doc = document;
    const clock = ElementById("c");
    const pClock = ElementById("pC");
    const pSettings = ElementById("pS");
    const pControls = ElementById("pCT");
    const pGameOver = ElementById("pGO");
    const pMastermind = ElementById("pMM");
    const pWordGuessr = ElementById("pWG");
    const color = ElementById("co");
    const speed = ElementById("speed");
    const wordInput = ElementById("wi");
    const rainbowMode =  ElementById("rm");
    const ghostMode = ElementById("gm");
    const darkMode = ElementById("dm");
    const body = doc.getElementsByTagName("body")[0];
    const codeBtns = ElementsByClassName("cdb");
    const colorBtns = ElementsByClassName("cb");
    const click = "click";
    const svgCircle = "<svg class='svgMsg' viewBox='0 0 70 70'> <circle cx='35' cy='35' r='25' fill=";
    const clockFace = "E g,S g,D,I g,S g,C g,H g,W,F M5,Ü M5,F M5,V M15,I M15,E M15,R M15,T M15,U M15,T,Z M10,Ä M10,Ä M10,Y,Z M20,W M20,Ä M20,N M20,Z M20,G M20,Q,D,V MV,O MV,R MV,K,A MA,B MA,D,H M30,A M30,U M30,B M30,I M30,T,Z,E H1,I H1,S H1,Q,Z H2,W H2,Ö H2,I H2,D H3,R H3,Ü H3,Z,V H4,I H4,E H4,R H4,I H4,F H5,Ü H5,F H5,I H5,T,G,M,S H6,Ä H6,C H6,H H6,S H6 H7,I H6 H7,B H7,N H7,I H7,A H8,C H8,H H8,T H8,I H8,N H9,Ü H9,N H9,I H9,O,F,C,D,Z H10,Ä H10,N H10,I H10,X,E H11,U H11,F H11,I H11,O,K,G,Z H0,W H0,Ö H0,U H0,F H0,I H0,L,X,L,Y,B,° M1,° M2,P,° M3,° M4,M,K";

    let date;
    let hour;
    let minute;
    let power = 1;
    let dark = 1;
    let ghost = 1;
    let rainbow = 0;
    let rainbowRed = 255;
    let rainbowGreen = 0;
    let rainbowBlue = 0;
    let game;
    let score = 0;
    let highscore = 0;
    let mastermindColor = "1";
    let mastermindWeiss = 0;
    let mastermindTry = 0;
    let wordGuessrScore = 0;
    let webSocket = null;

    function ElementById(id) {
        return doc.getElementById(id);
    }

    function ElementsByClassName(id) {
        return doc.getElementsByClassName(id);
    }

    function localStorageGet(key) {
        return localStorage.getItem(key);
    }

    function localStorageSet(key, value) {
        return localStorage.setItem(key, value);
    }

    function fClassList(element) {
        return element.classList;
    }

    function fChildren(element) {
        return element.children;
    }

    function fSetAttribute(element, attribute, value) {
        element.setAttribute(attribute, value);
    }

    function fEventListener(element, event, func) {
        element.addEventListener(event, func);
    }

    /**
     * Set the current time
     */
    function setTime() {
        date = new Date();
        if (dark && (date.getHours() >= 22 || date.getHours() < 7)) {
            fClassList(body).add("d");
        } else {
            fClassList(body).remove("d");
        }
        if (minute === date.getMinutes()) {
            return;
        }
        minute = date.getMinutes();
        fClassList(clock).remove(...fClassList(clock));
        if (power === 0) {
            return;
        }
        if (minute >= 55) {
            fClassList(clock).add("M5", "MV");
        } else if (minute >= 50) {
            fClassList(clock).add("M10", "MV");
        } else if (minute >= 45) {
            fClassList(clock).add("M15", "MV");
        } else if (minute >= 40) {
            fClassList(clock).add("M20", "MV");
        } else if (minute >= 35) {
            fClassList(clock).add("M5", "MA", "M30");
        } else if (minute >= 30) {
            fClassList(clock).add("M30");
        } else if (minute >= 25) {
            fClassList(clock).add("M5", "MV", "M30");
        } else if (minute >= 20) {
            fClassList(clock).add("M20", "MA");
        } else if (minute >= 15) {
            fClassList(clock).add("M15", "MA");
        } else if (minute >= 10) {
            fClassList(clock).add("M10", "MA");
        } else if (minute >= 5) {
            fClassList(clock).add("M5", "MA");
        }
        hour = date.getHours();
        if (minute >= 25) {
            hour += 1;
        }
        hour = hour % 12;
        fClassList(clock).add("H" + hour.toString());
        fClassList(clock).add("M" + (minute % 5).toString());
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
            fClassList(body).remove("off");
        } else {
            fClassList(body).add("off");
        }
        minute = -1;
        setTime();
    }

    /**
     * Show a page
     * @param {Element} pageShow : the page to show
     * @param {Element} pageHide : the page to hide
     */
    function fShowPage(pageHide, pageShow) {
        // Fix for Firefox OnKeydown
        doc.activeElement.blur();
        fClassList(pageHide).remove("show");
        fClassList(pageShow).remove("right");
        fClassList(pageHide).add("left");
        fClassList(pageShow).add("show");
    }

    /**
     * Hide a page
     * @param {Element} pageShow : the page to show
     * @param {Element} pageHide : the page to hide
     */
    function fHidePage(pageShow, pageHide) {
        fClassList(pageShow).remove("left");
        fClassList(pageHide).remove("show");
        fClassList(pageShow).add("show");
        fClassList(pageHide).add("right");
    }

    /**
     * Display the settings-page
     */
    function fShowSettings() {
        fShowPage(pClock, pSettings);
    }

    /**
     * Set all elements to selected color
     */
    function fChangeColor(color_in) {
        doc.documentElement.style.setProperty('--main-color', color_in);
    }

    /**
     * Set rainbow-mode on or off
     * @param {int} rain_in : 1 = rainbow on; 0 = rainbow off
     */
    function fRainbow(rain_in) {
        if (rain_in !== rainbow) {
            fClassList(fChildren(rainbowMode)[0]).toggle("h");
            fClassList(fChildren(rainbowMode)[1]).toggle("h");
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
            fClassList(fChildren(ghostMode)[0]).toggle("h");
            fClassList(fChildren(ghostMode)[1]).toggle("h");
        }
        ghost = ghost_in;
    }

    /**
     * Set dark-mode on or off
     * @param {int} dark_in : 1 = dark-mode on; 0 = dark-mode off
     */
    function fSetDarkMode(dark_in) {
        if (dark_in !== dark) {
            fClassList(fChildren(darkMode)[0]).toggle("h");
            fClassList(fChildren(darkMode)[1]).toggle("h");
        }
        dark = dark_in;
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
        fHidePage(pClock, pSettings);
        fClassList(pSettings).remove("sor");
        fUpdateParams();
    }

    /**
     * Hide the settings-page and return to clock-page
     */
    function fUpdateParams() {
        let red = parseInt(color.value.substring(1, 3), 16);
        let green = parseInt(color.value.substring(3, 5), 16);
        let blue = parseInt(color.value.substring(5, 7), 16);
        localStorageSet("wc_c", color.value);
        localStorageSet("wc_r", rainbow);
        localStorageSet("wc_d", dark);
        localStorageSet("wc_g", ghost);
        localStorageSet("wc_s", speed.value.toString());
        let xhr = new XMLHttpRequest();
        xhr.open("GET", "/update_params?red=" + red + "&green=" + green + "&blue=" + blue + "&rainbow=" + rainbow + "&darkmode=" + dark + "&speed=" + speed.value + "&power=" + power + "&ghost=" + ghost, true);
        xhr.send();
    }

    /**
     * Send game-control-input to word-clock
     * @param {string} cmd : direction for snake to move: 1=up, 2=right, 3=down, 4=left, 5=new game, 6=quit game
     */
    function fSendControls(cmd) {
        console.log(cmd);
        if (webSocket && webSocket.readyState === 1) {
            webSocket.send(cmd);
        }
    }

    /**
     * Display the controls-page for tetris / snake
     */
    function fShowControls() {
        pControls.classList.add(game);
        fShowPage(pSettings, pControls);
        fSendControls(game);
        if (localStorageGet("wc_" + game)) {
            highscore = localStorageGet("wc_" + game);
        } else {
            highscore = 0;
        }
    }

    /**
     * Hide the controls-page and return to settings-page
     */
    function fHideControls() {
        fHidePage(pSettings, pControls);
        fSendControls("stop");
        setTimeout(function () {
            fClassList(pControls).remove(game);
            game = "";
        }, 700);
    }

    /**
     * Display the game-over-page for tetris / snake
     */
    function fShowGameOver() {
        fShowPage(pControls, pGameOver);
    }

    /**
     * Hide the game-over-page, start a new game
     */
    function fPlayAgain() {
        fHidePage(pControls, pGameOver);
        fSendControls(game);
    }

    /**
     * Hide the game-over-page, start a new game
     */
    function fExitGame() {
        fHidePage(pControls, pGameOver);
        fHideControls();
        fHidePage(pClock, pSettings);
    }

    /**
     * Display the mastermind-page
     */
    function fShowMastermind() {
        fShowPage(pSettings, pMastermind);
        fSendMastermind(1);
    }

    /**
     * Send mastermind-control-input to word-clock
     * @param {int} action : 1=new game, 2=quit game, null=send try
     */
    function fSendMastermind(action) {
        let urlparams = "";
        if (action === 1) {
            urlparams = "mastermind?c4=0";
            fClearMastermind();
            fMastermindMessage();
        } else if (action === 2) {
            urlparams = "mastermind?c4=7"
            fClearMastermind();
        } else {
            if (doc.querySelectorAll(".cdb[data-num='1'], .cdb[data-num='2'], .cdb[data-num='3'], .cdb[data-num='4'], .cdb[data-num='5'], .cdb[data-num='6']").length < 4) {
                fMastermindMessage("Muesch zersch aues uswähle.");
                return;
            }
            urlparams = "mastermind?c1=" + codeBtns[0].getAttribute("data-num")
                + "&c2=" + codeBtns[1].getAttribute("data-num")
                + "&c3=" + codeBtns[2].getAttribute("data-num")
                + "&c4=" + codeBtns[3].getAttribute("data-num");
            fClearMastermind();
        }
        let xhttp = new XMLHttpRequest();
        xhttp.onreadystatechange = function () {
            if (this.readyState === 4 && this.status === 200) {
                let response = JSON.parse(xhttp.responseText);
                mastermindWeiss = response.place;
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
        fHidePage(pSettings, pMastermind);
        fSendMastermind(2);
    }

    /**
     * Clear current mastermind code
     */
    function fClearMastermind() {
        Array.from(codeBtns).forEach(function (element) {
            fSetAttribute(element, "data-num", "");
        });
    }

    /**
     * Show messages in Mastermine
     */
    function fMastermindMessage(msg) {
        if (msg) {
            ElementById("sMM").innerHTML = msg;
        } else {
            ElementById("sMM").innerHTML = svgCircle + "'white'/></svg>&nbsp;am richtige Ort&nbsp;" +
                svgCircle + "'cornflowerblue'/></svg>&nbsp;di richtigi Farb";
        }
    }

    /**
     * Display the wordguessr-page
     */
    function fShowWordGuessr() {
        fShowPage(pSettings, pWordGuessr);
        fSendWordGuessr("1");
        wordGuessrScore = 0;
        ElementById("sWG").innerHTML = "";
    }

    /**
     * Send wordguessr-control-input to word-clock
     * @param {string} word : "1"=new game, "2"=quit game, "word"=send try
     */
    function fSendWordGuessr(word) {
        let urlparams;
        if (word === "1") {
            urlparams = "wordguessr?new";
        } else if (word === "2") {
            urlparams = "wordguessr?exit"
        } else {
            urlparams = "wordguessr?word=" + wordInput.value;
        }
        let xhttp = new XMLHttpRequest();
        xhttp.onreadystatechange = function () {
            if (this.readyState === 4 && this.status === 200) {
                let response = JSON.parse(xhttp.responseText);
                if (response.score === 0) {
                    // wrong guess
                    fClassList(wordInput).add("error");
                    setTimeout(function () {
                        fClassList(wordInput).remove("error");
                        wordInput.value = "";
                    }, 100);
                } else if (response.score === 1) {
                    // correct guess
                    wordGuessrScore += response.score;
                    ElementById("sWG").innerHTML = wordGuessrScore + " hesch usegfunde.";
                    fClassList(wordInput).add("ok");
                    setTimeout(function () {
                        fClassList(wordInput).remove("ok");
                        wordInput.value = "";
                    }, 100);
                }
            }
        };
        xhttp.open("GET", urlparams, true);
        xhttp.send();
    }

    /**
     * Hide the wordguessr-page and return to settings-page
     */
    function fHideWordGuessr() {
        fHidePage(pSettings, pWordGuessr);
        fSendWordGuessr("2");
    }

    /**
     * Initialize application, add event-listeners
     */
    fEventListener(ElementById("p"), click, fTogglePower);
    fEventListener(ElementById("s"), click, fShowSettings);
    fEventListener(ElementById("xS"), click, fHideSettings);

    fEventListener(ElementById("SN"), click, function (e) {
        game = "snake";
        fShowControls();
    });
    fEventListener(ElementById("TE"), click, function (e) {
        game = "tetris";
        fShowControls();
    });
    fEventListener(ElementById("xCT"), click, fHideControls);
    fEventListener(ElementById("xGO"),click, fExitGame);
    fEventListener(ElementById("xGOA"),click, fPlayAgain);
    fEventListener(ElementById("MM"), click, fShowMastermind);
    fEventListener(ElementById("xMM"), click, fHideMastermind);
    fEventListener(ElementById("cMM"), click, fSendMastermind);
    fEventListener(ElementById("WG"), click, fShowWordGuessr);
    fEventListener(ElementById("xWG"), click, fHideWordGuessr);
    fEventListener(ElementById("cWG"), click, fSendWordGuessr);
    Array.from(ElementsByClassName("snb")).forEach(function (element) {
        fSetAttribute(element, "d", "M2 2 L9 7 L2 12 Z");
        fEventListener(element, click, function (e) {
            fSendControls(e.target.getAttribute("data-dir"));
        });
    });
    // no-svg: x
    Array.from(ElementsByClassName("n")).forEach(function (element) {
        fSetAttribute(element, "d", "M10 20 L20 10 L35 25 L50 10 L60 20 L45 35 L60 50 L50 60 L35 45 L20 60 L10 50 L25 35 L10 20 Z");
        fSetAttribute(element, "transform", "scale(0.9) translate(5,5)");
    });
    // yes-svg: check
    Array.from(ElementsByClassName("y")).forEach(function (element) {
        fSetAttribute(element, "d", "M0 40 L10 30 L20 40 L50 10 L60 20 L20 60 L0 40 Z");
        fSetAttribute(element, "transform", "scale(0.85) translate(5,5)");
    });
    // play-svg: >
    Array.from(ElementsByClassName("play")).forEach(function (element) {
        element.innerHTML = "<path d='M2 2 L9 7 L2 12 Z' stroke-width='1.4'/>";
    });
    Array.from(colorBtns).forEach(function (element) {
        fEventListener(element, click, function (e) {
            Array.from(colorBtns).forEach(function (element) {
                fClassList(element).remove("g");
            });
            fClassList(e.target).add("g");
            mastermindColor = e.target.getAttribute("data-num");
        });
        element.innerHTML = "<circle cx='35' cy='35' r='25'/>";
        fSetAttribute(element, "viewBox", "0 0 70 70");
    });
    Array.from(codeBtns).forEach(function (element) {
        fEventListener(element, click, function (e) {
            fSetAttribute(e.target, "data-num", mastermindColor);
            fMastermindMessage()
        });
        element.innerHTML = "<circle cx='35' cy='35' r='25'/>";
        fSetAttribute(element, "viewBox", "0 0 70 70");
    });

    function fCheckKey(e) {
        let dir = "";
        switch (e.key) {
            case "ArrowUp":
                dir = "up";
                break;
            case "ArrowRight":
                dir = "right";
                break;
            case "ArrowDown":
                dir = "down";
                break;
            case "ArrowLeft":
                dir = "left";
                break;
            case "Enter":
                if ( fClassList(pWordGuessr).contains("show")) {
                    fSendWordGuessr();
                }
        }
        if (dir && game) {
            fSendControls(dir);
            if (dir == "up" && game == "tetris") {
                dir = "tup";
            }
            fClassList(ElementById("ctrl" + dir)).add("g");
            setTimeout(function () {
                fClassList(ElementById("ctrl" + dir)).remove("g");
            }, 200);
        }
    }

    doc.onkeydown = fCheckKey;
    fEventListener(color, "change", (ignore) => {
        fChangeColor(color.value);
    }, false);
    fEventListener(rainbowMode, click, (ignore) => {
        fRainbow(1 - rainbow);
    });
    fEventListener(ghostMode, click, (ignore) => {
        fGhost(1 - ghost);
    });
    fEventListener(darkMode, click, (ignore) => {
        fSetDarkMode(1 - dark);
    });

    /**
     * Reload last settings from local storage
     */
    if (localStorageGet("wc_c")) {
        color.value = localStorageGet("wc_c");
        fChangeColor(color.value);
    }
    if (localStorageGet("wc_r")) {
        fRainbow(parseInt(localStorageGet("wc_r")));
    }
    if (localStorageGet("wc_g")) {
        fGhost(parseInt(localStorageGet("wc_g")));
    }
    if (localStorageGet("wc_d")) {
        fSetDarkMode(parseInt(localStorageGet("wc_d")));
    }
    if (localStorageGet("wc_s")) {
        speed.value = (parseInt(localStorageGet("wc_s")));
    }
    ElementById("iphone").href = ElementById("icon").href;

    clockFace.split(",").forEach(function(element, index1) {
        const textElement = doc.createElementNS("http://www.w3.org/2000/svg", "text");
        fSetAttribute(textElement, "x", (index1 % 11 * 10) + 7);
        fSetAttribute(textElement, "y", Math.ceil((index1 + 1) / 11) * 10);
        if ([113,114,116,117].includes(index1)) {
            fSetAttribute(textElement, "y", 112.5);
        }
        element.split(" ").forEach(function (element2, index2) {
            if (!index2) {
                textElement.textContent = element2;
            } else {
                fClassList(textElement).add(element2);
            }
        });
        clock.appendChild(textElement);
    });

    // generate Titles on Pages (grrd: ewfGRRDcSaj, mascha: qwMASCHAbSd)
    const pageTitleLine1 = "ewfGRRDcSaj";
    const pageTitles = [pageTitleLine1 + "nWORDuCLOCK", pageTitleLine1 + "mSNAKExlbdk", pageTitleLine1 + "mTETRISlbdk", "ewfGAMEcsajmsnakOVERdk", pageTitleLine1 + "MASTERMINDk", pageTitleLine1 + "WORDbGUESSR"];
    Array.from(ElementsByClassName("t")).forEach(function (element, index) {
        for (let step = 0; step < 22; step++) {
            const textElement = doc.createElementNS("http://www.w3.org/2000/svg", "text");
            let letter = pageTitles[index].substring(step, step + 1);
            let letterUpper = letter.toUpperCase();
            fSetAttribute(textElement, "x", (step % 11 * 10) + 7);
            fSetAttribute(textElement, "y", Math.ceil((step + 1) / 11) * 10);
            textElement.textContent = letterUpper;
            if (letter === letterUpper) {
                fSetAttribute(textElement, "class", "g");
            }
            element.appendChild(textElement);
        }
    });

    // initialize websocket connection for game controls
    try {
        webSocket = new WebSocket('ws://' + location.hostname + ':81/');
        webSocket.onopen = function(){ console.log('webSocket open'); };
        webSocket.onmessage = function(e) {
            if (e.data && e.data.indexOf('score:') === 0) {
                score = parseInt(e.data.split(':')[1]);
                if (score > highscore) {
                    highscore = score;
                    localStorageSet("wc_" + game, highscore);
                }
                ElementById("sCT").innerHTML = "Score: " + score + " / High-Score : " + highscore;
            }
            if (e.data && e.data.indexOf('gameOver') === 0) {
                ElementById("sGO").innerHTML = score;
                ElementById("hsGO").innerHTML = highscore;
                fShowGameOver();
            }
        }
        webSocket.onclose = function(){ console.log('webSocket closed'); };
        webSocket.onerror = function(e){ console.log('webSocket error', e); };
    } catch(e) { console.log('webSocket init failed'); }

    /**
     * Load current settings from word-clock
     */
    let xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
        if (this.readyState === 4 && this.status === 200) {
            let response = JSON.parse(xhttp.responseText);
            if (!response.rainbow) {
                color.value = fRgb2Hex(response.red, response.green, response.blue);
            }
            fChangeColor(color.value);
            fSetDarkMode(response.darkmode);
            fRainbow(response.rainbow);
            fGhost(response.ghost);
            fSetPower(response.power);
            speed.value = response.speed;
        }
    };
    xhttp.open("GET", "get_params", true);
    xhttp.send();
}())
