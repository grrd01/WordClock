(function () {
	"use strict";

	const pages = Array.from(document.querySelectorAll(".p"));
	const prevButton = document.querySelector("#prev");
	const nextButton = document.querySelector("#next");
	const pageIndicator = document.querySelector("#pageIndicator");
	const pageNames = pages.map(function (page) {
		return page.id.replace(/^page/i, "").toLowerCase();
	});
	const pageLookup = new Map();
	let minute = -1;
	let currentPageIndex = 0;


	if (pages.length === 0) {
		return;
	}

	pages.forEach(function (page, index) {
		pageLookup.set(page.id.toLowerCase(), index);
		pageLookup.set(pageNames[index], index);
	});

	function updatePageIndicator() {
		if (pageIndicator) {
			pageIndicator.textContent = (currentPageIndex + 1).toString() + "/" + pages.length.toString();
		}
	}

	function updatePageUrl() {
		const url = new URL(window.location.href);
		url.searchParams.set("page", pageNames[currentPageIndex]);
		window.history.replaceState(null, "", url.toString());
	}

	function getPageIndexFromParam(value) {
		if (!value) {
			return 0;
		}

		const normalizedValue = value.toLowerCase().replace(/^page/i, "");
		if (pageLookup.has(normalizedValue)) {
			return pageLookup.get(normalizedValue);
		}

		if (pageLookup.has(value.toLowerCase())) {
			return pageLookup.get(value.toLowerCase());
		}

		return 0;
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

		if (prevButton) {
			prevButton.hidden = currentPageIndex === 0;
			prevButton.setAttribute("aria-hidden", prevButton.hidden.toString());
		}

		if (nextButton) {
			nextButton.hidden = currentPageIndex === pages.length - 1;
			nextButton.setAttribute("aria-hidden", nextButton.hidden.toString());
		}

		updatePageIndicator();
		updatePageUrl();
	}

	showPage(getPageIndexFromParam((new URL(window.location.href)).searchParams.get("page")));

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

}());
