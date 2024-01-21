{
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
    const color = document.getElementById("color");
    function setTime () {
        date = new Date();
        hour = date.getHours() % 12;
        alert("settime3a");
        if (rainbow === 1) {
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
        }
        alert("settime3b");
        if (darkMode === 1 && (date.getHours() >=22 || date.getHours() < 7)) {
            document.getElementsByTagName("body")[0].classList.add("dark");
        } else {
            document.getElementsByTagName("body")[0].classList.remove("dark");
        }
        alert("settime3c");
        if (minute === date.getMinutes()) {
            return;
        }
        alert("settime3d");
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
        if (minute >= 25) {
            hour += 1;
        }
        clock.classList.add("H" + hour.toString());
        clock.classList.add("M" + (minute % 5).toString());
    }
    setInterval(setTime, 100);

    function fShowSettings() {
        // Fix for Firefox OnKeydown
        document.activeElement.blur();
        pageClock.classList.remove("swipe-out-right");
        pageSettings.classList.remove("swipe-in-left");
        pageClock.classList.add("swipe-out");
        pageSettings.classList.add("swipe-in");
    }
    function fChangeColor(color_in) {
        document.styleSheets[0].cssRules[3].style.fill = color_in;
        document.styleSheets[0].cssRules[3].style.textShadow = "0 0 10px " + color_in;
        document.getElementsByTagName("a")[0].style.color = color_in;
        document.getElementsByTagName("a")[1].style.color = color_in;
    }
    function fRainbow(rain_in) {
        if (rain_in !== rainbow) {
            document.getElementById("rainbowMode").children[0].classList.toggle("hide");
            document.getElementById("rainbowMode").children[1].classList.toggle("hide");
        }
        rainbow = rain_in;
        if (!rainbow) {
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
    function fHideSettings() {
        let red = parseInt(color.value.substring(1,3), 16);
        let green = parseInt(color.value.substring(3,5), 16);
        let blue = parseInt(color.value.substring(5,7), 16);
        pageClock.classList.remove("swipe-out");
        pageSettings.classList.remove("swipe-in");
        pageClock.classList.add("swipe-out-right");
        pageSettings.classList.add("swipe-in-left");
        localStorage.setItem("wc_color", color.value);
        localStorage.setItem("wc_rainbow", rainbow);
        localStorage.setItem("wc_darkmode", darkMode);
        setTimeout(function() {
            window.location.search = ("&red=" + red + "&green=" + green + "&blue=" + blue + "&rainbow=" + rainbow + "&darkmode=" + darkMode);
        }, 500);

    }
    document.getElementById("settings").addEventListener("click", fShowSettings);
    document.getElementById("settingsClose").addEventListener("click", fHideSettings);
    document.getElementById("color").addEventListener("change", (ignore) => {
        fChangeColor(color.value);
    }, false);
    document.getElementById("rainbowMode").addEventListener("click", (ignore) => {
        fRainbow(1 - rainbow);
    });
    document.getElementById("darkMode").addEventListener("click", (ignore) => {
        fDarkMode(1 - darkMode);
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
    document.getElementById("iphone").href = document.getElementById("icon").href;
}
