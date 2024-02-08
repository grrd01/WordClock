(function () {
    "use strict";

    let date;
    let hour;
    let minute;
    let darkMode = 1;
    let rainbow = 0;
    let rainbowRed = 255;
    let rainbowGreen = 0;
    let rainbowBlue = 0;
    const clock = document.getElementById("clock");
    const pageClock = document.getElementById("pageClock");
    const pageSettings = document.getElementById("pageSettings");
    const pageSnake = document.getElementById("pageSnake");
    const color = document.getElementById("color");
    const speed = document.getElementById("speed");
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

    function fShowSettings() {
        // Fix for Firefox OnKeydown
        document.activeElement.blur();
        pageClock.classList.remove("swipe-out-right");
        pageSettings.classList.remove("swipe-in-left");
        pageClock.classList.add("swipe-out");
        pageSettings.classList.add("swipe-in");
    }
    function fChangeColor(color_in) {
        document.documentElement.style.setProperty('--main-color', color_in);
    }
    function fRainbow(rain_in) {
        if (rain_in !== rainbow) {
            document.getElementById("rainbowMode").children[0].classList.toggle("hide");
            document.getElementById("rainbowMode").children[1].classList.toggle("hide");
        }
        rainbow = rain_in;
        if (rainbow) {
            fChangeRainbow();
        } else {
            fChangeColor(color.value);
        }
    }
    function fDarkMode(dark_in) {
        if (dark_in !== darkMode) {
            document.getElementById("darkMode").children[0].classList.toggle("hide");
            document.getElementById("darkMode").children[1].classList.toggle("hide");
        }
        darkMode = dark_in;
    }
    function fRgb2Hex(r, g, b) {
        return "#" + (1 << 24 | r << 16 | g << 8 | b).toString(16).slice(1);
    }
    function fHideSettings() {
        let red = parseInt(color.value.substring(1,3), 16);
        let green = parseInt(color.value.substring(3,5), 16);
        let blue = parseInt(color.value.substring(5,7), 16);
        pageClock.classList.remove("swipe-out");
        pageSettings.classList.remove("swipe-in");
        pageSettings.classList.remove("swipe-out-right")
        pageClock.classList.add("swipe-out-right");
        pageSettings.classList.add("swipe-in-left");
        localStorage.setItem("wc_color", color.value);
        localStorage.setItem("wc_rainbow", rainbow);
        localStorage.setItem("wc_darkmode", darkMode);
        localStorage.setItem("wc_speed", speed.value.toString());
        if (window.location.href.includes("192.168.")) {
            let xhr = new XMLHttpRequest();
            xhr.open("GET", "/update_params?red=" + red + "&green=" + green + "&blue=" + blue + "&rainbow=" + rainbow + "&darkmode=" + darkMode + "&speed=" + speed.value, true);
            xhr.send();
        }
    }
    function fFindWordClock (ip1, ip2) {
        // ping ip-adress to check for wordClock in local network
        let xhttp = new XMLHttpRequest();
        xhttp.onreadystatechange = function() {
            if (this.readyState === 4) {
                if (this.status === 200 && xhttp.responseText.startsWith("WORDCLOCK")) {
                    document.getElementById("searchLabel").innerHTML = "Ha se gfunde: http://192.168." + ip1 + "." + ip2;
                    setTimeout(function() {
                        window.open("http://192.168." + ip1 + "." + ip2,"_self");
                    }, 3000);

                } else {
                    ip2++;
                    if (ip2 === 256) {
                        ip1++;
                        ip2 = 0;
                    }
                    if (ip1 < 256) {
                        document.getElementById("searchLabel").innerHTML = "Hie isch si nid: http://192.168." + ip1 + "." + ip2;
                        fFindWordClock(ip1, ip2);
                    }
                }
            }
        };
        xhttp.timeout = 300;
        //xhttp.ontimeout = function () { alert("Timed out!!!"); }
        xhttp.open("GET", "http://192.168." + ip1 + "." + ip2 + "/ping", true);
        xhttp.send();
    }
    function fShowSnake() {
        // Fix for Firefox OnKeydown
        document.activeElement.blur();
        pageSettings.classList.remove("swipe-out-right");
        pageSnake.classList.remove("swipe-in-left");
        pageSettings.classList.add("swipe-out");
        pageSnake.classList.add("swipe-in");
        fSendSnake (5);
    }
    function fSendSnake (dir) {
        let xhttp = new XMLHttpRequest();
        xhttp.onreadystatechange = function() {
            if (this.readyState === 4 && this.status === 200) {
                //let response = JSON.parse(xhttp.responseText);
                console.log("current score" + xhttp.responseText);
            }
        };
        xhttp.open("GET", "snake?dir=" + dir, true);
        xhttp.send();
    }
    function fHideSnake() {
        pageSettings.classList.remove("swipe-out");
        pageSnake.classList.remove("swipe-in");
        pageSettings.classList.add("swipe-out-right");
        pageSnake.classList.add("swipe-in-left");
        fSendSnake (6);
    }
    document.getElementById("settings").addEventListener("click", fShowSettings);
    document.getElementById("settingsClose").addEventListener("click", fHideSettings);
    document.getElementById("searchClock").addEventListener("click", (ignore) => {
        //document.getElementById("searchClock").removeEventListener("click");
        document.getElementById("searchClock").style.pointerEvents = "none";
        // 192.168. 0.0 is the beginning of the private IP address range that includes all IP addresses through 192.168. 255.255.
        fFindWordClock(0, 0);
    });
    document.getElementById("playSnake").addEventListener("click", fShowSnake);
    document.getElementById("exitSnake").addEventListener("click", fHideSnake);
    Array.from(document.getElementsByClassName("snakeButton")).forEach(function(element) {
        element.addEventListener("click", function (e) {
            fSendSnake(e.target.getAttribute("data-num"));
        });
    });
    color.addEventListener("change", (ignore) => {
        fChangeColor(color.value);
    }, false);
    document.getElementById("rainbowMode").addEventListener("click", (ignore) => {
        fRainbow(1 - rainbow);
    });
    document.getElementById("darkMode").addEventListener("click", (ignore) => {
        fDarkMode(1 - darkMode);
    });
    speed.addEventListener("change", (ignore) => {
        console.log(speed.value);
    });
    if (localStorage.getItem("wc_color")) {
        color.value = localStorage.getItem("wc_color");
        fChangeColor(color.value);
    }
    if (localStorage.getItem("wc_rainbow")) {
        fRainbow(parseInt(localStorage.getItem("wc_rainbow")));
    }
    if (localStorage.getItem("wc_darkmode")) {
        fDarkMode(parseInt(localStorage.getItem("wc_darkmode")));
    }
    if (localStorage.getItem("wc_speed")) {
        speed.value = (parseInt(localStorage.getItem("wc_speed")));
    }
    document.getElementById("iphone").href = document.getElementById("icon").href;

    // load current params from clock
    if (window.location.href.includes("192.168.")) {
        document.getElementById("searchBody").classList.add("hide");
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
                speed.value = response.speed;
            }
        };
        xhttp.open("GET", "get_params", true);
        xhttp.send();
    }

}());
