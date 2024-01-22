{
    let a, l, d, o = 1, c = 0, n = 255, i = 0, r = 0;
    const g = document.getElementById("clock"), m = document.getElementById("pageClock"),
        u = document.getElementById("pageSettings"), L = document.getElementById("color");

    function e(e) {
        document.styleSheets[0].cssRules[3].style.fill = e, document.styleSheets[0].cssRules[3].style.textShadow = "0 0 10px " + e, document.getElementsByTagName("a")[0].style.color = e, document.getElementsByTagName("a")[1].style.color = e
    }

    function t(t) {
        t !== c && (document.getElementById("rainbowMode").children[0].classList.toggle("hide"), document.getElementById("rainbowMode").children[1].classList.toggle("hide")), c = t, c || e(L.value)
    }

    function s(e) {
        e !== o && (document.getElementById("darkMode").children[0].classList.toggle("hide"), document.getElementById("darkMode").children[1].classList.toggle("hide")), o = e
    }

    setInterval((function () {
        a = new Date, l = a.getHours() % 12, c && (n && !r ? (n -= 1, i += 1) : i ? (i -= 1, r += 1) : (r -= 1, n += 1), e("rgb(" + n + ", " + i + ", " + r + ")")), o && (a.getHours() >= 22 || a.getHours() < 7) ? document.getElementsByTagName("body")[0].classList.add("dark") : document.getElementsByTagName("body")[0].classList.remove("dark"), d !== a.getMinutes() && (d = a.getMinutes(), g.classList.remove(...g.classList), d >= 55 ? g.classList.add("M5", "MV") : d >= 50 ? g.classList.add("M10", "MV") : d >= 45 ? g.classList.add("M15", "MV") : d >= 40 ? g.classList.add("M20", "MV") : d >= 35 ? g.classList.add("M5", "MA", "M30") : d >= 30 ? g.classList.add("M30") : d >= 25 ? g.classList.add("M5", "MV", "M30") : d >= 20 ? g.classList.add("M20", "MA") : d >= 15 ? g.classList.add("M15", "MA") : d >= 10 ? g.classList.add("M10", "MA") : d >= 5 && g.classList.add("M5", "MA"), d >= 25 && (l += 1), g.classList.add("H" + l.toString()), g.classList.add("M" + (d % 5).toString()))
    }), 100), document.getElementById("settings").addEventListener("click", (function () {
        document.activeElement.blur(), m.classList.remove("swipe-out-right"), u.classList.remove("swipe-in-left"), m.classList.add("swipe-out"), u.classList.add("swipe-in")
    })), document.getElementById("settingsClose").addEventListener("click", (function () {
        let e = parseInt(L.value.substring(1, 3), 16), t = parseInt(L.value.substring(3, 5), 16),
            s = parseInt(L.value.substring(5, 7), 16);
        m.classList.remove("swipe-out"), u.classList.remove("swipe-in"), m.classList.add("swipe-out-right"), u.classList.add("swipe-in-left"), localStorage.setItem("wc_color", L.value), localStorage.setItem("wc_rainbow", c), localStorage.setItem("wc_darkmode", o), setTimeout((function () {
            window.location.search = "&red=" + e + "&green=" + t + "&blue=" + s + "&rainbow=" + c + "&darkmode=" + o
        }), 500)
    })), document.getElementById("color").addEventListener("change", t => {
        e(L.value)
    }, !1), document.getElementById("rainbowMode").addEventListener("click", e => {
        t(1 - c)
    }), document.getElementById("darkMode").addEventListener("click", e => {
        s(1 - o)
    }), localStorage.getItem("wc_color") && (L.value = localStorage.getItem("wc_color"), e(L.value)), localStorage.getItem("wc_rainbow") && t(parseInt(localStorage.getItem("wc_rainbow"))), localStorage.getItem("wc_darkmode") && s(parseInt(localStorage.getItem("wc_darkmode")))
}