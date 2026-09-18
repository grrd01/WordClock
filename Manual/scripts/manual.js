(function () {
	"use strict";

	const body = document.body;
	const clock = document.querySelector("#pageWordClock .t");
	const pages = Array.from(document.querySelectorAll(".p"));
	const prevButton = document.querySelector("#prev");
	const nextButton = document.querySelector("#next");
	const dynamicClockClasses = [
		"M0", "M1", "M2", "M3", "M4",
		"M5", "M10", "M15", "M20", "M30", "MA", "MV",
		"H0", "H1", "H2", "H3", "H4", "H5", "H6", "H7", "H8", "H9", "H10", "H11"
	];
	let minute = -1;
	let currentPageIndex = 0;

	if (!clock) {
		return;
	}

	if (pages.length === 0) {
		return;
	}

	function showPage(index) {
		currentPageIndex = (index + pages.length) % pages.length;

		pages.forEach(function (page, pageIndex) {
			page.classList.remove("left", "show", "right");

			if (pageIndex < currentPageIndex) {
				page.classList.add("left");
			} else if (pageIndex === currentPageIndex) {
				page.classList.add("show");
			} else {
				page.classList.add("right");
			}
		});
	}

	showPage(0);

	if (prevButton) {
		prevButton.addEventListener("click", function () {
			showPage(currentPageIndex - 1);
		});
	}

	if (nextButton) {
		nextButton.addEventListener("click", function () {
			showPage(currentPageIndex + 1);
		});
	}

	function setTime() {
		const date = new Date();

		if (date.getHours() >= 22 || date.getHours() < 7) {
			body.classList.add("d");
		} else {
			body.classList.remove("d");
		}

		if (minute === date.getMinutes()) {
			return;
		}

		minute = date.getMinutes();
		clock.classList.remove(...dynamicClockClasses);

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

		let hour = date.getHours();
		if (minute >= 25) {
			hour += 1;
		}
		hour %= 12;

		clock.classList.add("H" + hour.toString());
		clock.classList.add("M" + (minute % 5).toString());
	}

	setTime();
	setInterval(setTime, 100);
}());

