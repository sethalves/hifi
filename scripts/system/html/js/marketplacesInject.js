//
//  marketplacesInject.js
//
//  Created by David Rowe on 12 Nov 2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Injected into marketplace Web pages.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function () {

    function injectCommonCode(isDirectoryPage) {

        // Supporting styles from marketplaces.css.
        // Glyph font family, size, and spacing adjusted because HiFi-Glyphs cannot be used cross-domain.
        $("head").append(
            '<style>' +
                '#marketplace-navigation { font-family: Arial, Helvetica, sans-serif; width: 100%; height: 50px; background: #00b4ef; position: fixed; bottom: 0; }' +
                '#marketplace-navigation .glyph { margin-left: 20px; margin-right: 3px; font-family: sans-serif; color: #fff; font-size: 24px; line-height: 50px; }' +
                '#marketplace-navigation .text { color: #fff; font-size: 18px; line-height: 50px; vertical-align: top; position: relative; top: 1px; }' +
                '#marketplace-navigation input#back-button { position: absolute; left: 20px; margin-top: 12px; padding-left: 0; padding-right: 5px; }' +
                '#marketplace-navigation input#all-markets { position: absolute; right: 20px; margin-top: 12px; padding-left: 15px; padding-right: 15px; }' +
            '</style>'
        );

        // Supporting styles from edit-style.css.
        // Font family, size, and position adjusted because Raleway-Bold cannot be used cross-domain.
        $("head").append(
            '<style>' +
                'input[type=button] { font-family: Arial, Helvetica, sans-serif; font-weight: bold; font-size: 12px; text-transform: uppercase; vertical-align: center; height: 28px; min-width: 100px; padding: 0 15px; border-radius: 5px; border: none; color: #fff; background-color: #000; background: linear-gradient(#343434 20%, #000 100%); cursor: pointer; }' +
                'input[type=button].white { color: #121212; background-color: #afafaf; background: linear-gradient(#fff 20%, #afafaf 100%); }' +
                'input[type=button].white:enabled:hover { background: linear-gradient(#fff, #fff); border: none; }' +
                'input[type=button].white:active { background: linear-gradient(#afafaf, #afafaf); }' +
            '</style>'
        );

        // Footer.
        var isInitialDirectoryPage = location.href.match(/\/scripts\/system\/html\/marketplaces\.html$/);
        $("body").append(
            '<div id="marketplace-navigation">' +
                (isInitialDirectoryPage ? '<span class="glyph">&#x1f6c8;</span> <span class="text">Select a marketplace to explore.</span>' : '') +
                (!isInitialDirectoryPage ? '<input id="back-button" type="button" class="white" value="&lt; Back" />' : '') +
                (!isDirectoryPage ? '<input id="all-markets" type="button" class="white" value="See All Markets" />' : '') +
            '</div>'
        );

        // Footer actions.
        $("#back-button").on("click", function () {
            window.history.back();
        });
        $("#all-markets").on("click", function () {
            EventBridge.emitWebEvent("GOTO_DIRECTORY");
        });
    }

    function injectDirectoryCode() {

        // Remove e-mail hyperlink.
        var letUsKnow = $("#letUsKnow");
        letUsKnow.replaceWith(letUsKnow.html());

        // Add button links.
        $('#exploreClaraMarketplace').on('click', function () {
            window.location = "https://clara.io/library?gameCheck=true&public=true"
        });
        $('#exploreHifiMarketplace').on('click', function () {
            window.location = "http://www.highfidelity.com/marketplace"
        });
    }

    function injectHiFiCode() {
        // Nothing to do.
    }

    function updateClaraCode(currentLocation) {
        // Have to manually monitor location for changes because Clara Web page replaced content rather than loading new page.

        if (location.href !== currentLocation) {

            // Clara library page.
            if (location.href.indexOf("clara.io/library") !== -1) {
                // Make entries navigate to "Image" view instead of default "Real Time" view.
                var elements = $("a.thumbnail");
                for (var i = 0, length = elements.length; i < length; i++) {
                    var value = elements[i].getAttribute("href");
                    if (value.slice(-6) !== "/image") {
                        elements[i].setAttribute("href", value + "/image");
                    }
                }
            }

            // Clara item page.
            if (location.href.indexOf("clara.io/view/") !== -1) {
                // Make site navigation links retain gameCheck etc. parameters.
                var element = $("a[href^=\'/library\']")[0];
                var parameters = "?gameCheck=true&public=true";
                var href = element.getAttribute("href");
                if (href.slice(-parameters.length) !== parameters) {
                    element.setAttribute("href", href + parameters);
                }

                // Replace download options with a single, "Download to High Fidelity" option.
                var buttons = $("a.embed-button").parent("div");
                if (buttons.length > 0) {
                    var downloadFBX = buttons.find("a[data-extension=\'fbx\']")[0];
                    downloadFBX.addEventListener("click", startAutoDownload);
                    var firstButton = buttons.children(":first-child")[0];
                    buttons[0].insertBefore(downloadFBX, firstButton);
                    downloadFBX.setAttribute("class", "btn btn-primary download");
                    downloadFBX.innerHTML = "<i class=\'glyphicon glyphicon-download-alt\'></i> Download to High Fidelity";
                    buttons.children(":nth-child(2), .btn-group , .embed-button").each(function () { this.remove(); });
                }

                // Automatic download to High Fidelity.
                var downloadTimer;
                function startAutoDownload() {
                    if (!downloadTimer) {
                        downloadTimer = setInterval(autoDownload, 1000);
                    }
                }
                function autoDownload() {
                    if ($("div.download-body").length !== 0) {
                        var downloadButton = $("div.download-body a.download-file");
                        if (downloadButton.length > 0) {
                            clearInterval(downloadTimer);
                            downloadTimer = null;
                            var href = downloadButton[0].href;
                            EventBridge.emitWebEvent("CLARA.IO DOWNLOAD " + href);
                            console.log("Clara.io FBX file download initiated for " + href);
                            $("a.btn.cancel").click();
                        };
                    } else {
                        clearInterval(downloadTimer);
                        downloadTimer = null;
                    }
                }
            }

            currentLocation = location.href;
        }
    }

    function injectClaraCode() {

        // Make space for marketplaces footer in Clara pages.
        $("head").append(
            '<style>' +
                '#app { margin-bottom: 135px; }' +
                '.footer { bottom: 50px; }' +
            '</style>'
        );

        // Update code injected per page displayed.
        var currentLocation = "";
        var checkLocationInterval = undefined;
        updateClaraCode(currentLocation);
        checkLocationInterval = setInterval(function () {
            updateClaraCode(currentLocation);
        }, 1000);

        window.addEventListener("unload", function () {
            clearInterval(checkLocationInterval);
            checkLocationInterval = undefined;
            currentLocation = "";
        });
    }

    function onLoad() {
        var DIRECTORY = 0;
        var HIFI = 1;
        var CLARA = 2;
        var pageType = DIRECTORY;

        if (location.href.indexOf("highfidelity.com/") !== -1) { pageType = HIFI; }
        if (location.href.indexOf("clara.io/") !== -1) { pageType = CLARA; }

        injectCommonCode(pageType === DIRECTORY);
        switch (pageType) {
            case DIRECTORY:
                injectDirectoryCode();
                break;
            case HIFI:
                injectHiFiCode();
                break;
            case CLARA:
                injectClaraCode();
                break;
        }
    }

    // Load / unload.
    try {
        // This appears more responsive to the user but $ is not necessarily loaded in time for each marketplace.
        $(document).ready(function () { onLoad(); });
    }
    catch (e) {
        window.addEventListener("load", onLoad);
    }

}());
