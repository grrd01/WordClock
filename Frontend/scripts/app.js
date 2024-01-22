{
    let date;
    let hour;
    let minute;
    const clock = document.getElementById("clock");
    const pageClock = document.getElementById("pageClock");
    const pageSettings = document.getElementById("pageSettings");
    const color = document.getElementById("color");
    function setTime () {
        date = new Date();
        hour = date.getHours() % 12;
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
    function fChangeColor() {
        document.styleSheets[0].cssRules[3].style.fill = color.value;
        document.styleSheets[0].cssRules[3].style.textShadow = "0 0 10px " + color.value;
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
        window.location.search = ("&red=" + red + "&green=" + green + "&blue=" + blue);
    }
    document.getElementById("settings").addEventListener("click", fShowSettings);
    document.getElementById("settingsClose").addEventListener("click", fHideSettings);
    document.getElementById("color").addEventListener("change", fChangeColor, false);
    if (localStorage.getItem("wc_color")) {
        color.value = localStorage.getItem("wc_color");
        fChangeColor();
    }
}