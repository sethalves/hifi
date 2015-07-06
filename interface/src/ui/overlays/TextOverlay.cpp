//
//  TextOverlay.cpp
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// include this before QGLWidget, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include "TextOverlay.h"

#include <DependencyManager.h>
#include <GeometryCache.h>
#include <RegisteredMetaTypes.h>
#include <SharedUtil.h>
#include <TextRenderer.h>


TextOverlay::TextOverlay() :
    _backgroundColor(DEFAULT_BACKGROUND_COLOR),
    _backgroundAlpha(DEFAULT_BACKGROUND_ALPHA),
    _leftMargin(DEFAULT_MARGIN),
    _topMargin(DEFAULT_MARGIN),
    _fontSize(DEFAULT_FONTSIZE)
{
    _textRenderer = TextRenderer::getInstance(SANS_FONT_FAMILY, _fontSize, DEFAULT_FONT_WEIGHT);
}

TextOverlay::TextOverlay(const TextOverlay* textOverlay) :
    Overlay2D(textOverlay),
    _text(textOverlay->_text),
    _backgroundColor(textOverlay->_backgroundColor),
    _backgroundAlpha(textOverlay->_backgroundAlpha),
    _leftMargin(textOverlay->_leftMargin),
    _topMargin(textOverlay->_topMargin),
    _fontSize(textOverlay->_fontSize)
{
    _textRenderer = TextRenderer::getInstance(SANS_FONT_FAMILY, _fontSize, DEFAULT_FONT_WEIGHT);
}

TextOverlay::~TextOverlay() {
    delete _textRenderer;
}

xColor TextOverlay::getBackgroundColor() {
    if (_colorPulse == 0.0f) {
        return _backgroundColor; 
    }

    float pulseLevel = updatePulse();
    xColor result = _backgroundColor;
    if (_colorPulse < 0.0f) {
        result.red *= (1.0f - pulseLevel);
        result.green *= (1.0f - pulseLevel);
        result.blue *= (1.0f - pulseLevel);
    } else {
        result.red *= pulseLevel;
        result.green *= pulseLevel;
        result.blue *= pulseLevel;
    }
    return result;
}


void TextOverlay::render(RenderArgs* args) {
    if (!_visible) {
        return; // do nothing if we're not visible
    }

    const float MAX_COLOR = 255.0f;
    xColor backgroundColor = getBackgroundColor();
    glm::vec4 quadColor(backgroundColor.red / MAX_COLOR, backgroundColor.green / MAX_COLOR, backgroundColor.blue / MAX_COLOR,
        getBackgroundAlpha());

    int left = _bounds.left();
    int right = _bounds.right() + 1;
    int top = _bounds.top();
    int bottom = _bounds.bottom() + 1;

    glm::vec2 topLeft(left, top);
    glm::vec2 bottomRight(right, bottom);
    glBindTexture(GL_TEXTURE_2D, 0);
    DependencyManager::get<GeometryCache>()->renderQuad(topLeft, bottomRight, quadColor);
    
    const int leftAdjust = -1; // required to make text render relative to left edge of bounds
    const int topAdjust = -2; // required to make text render relative to top edge of bounds
    int x = _bounds.left() + _leftMargin + leftAdjust;
    int y = _bounds.top() + _topMargin + topAdjust;
    
    float alpha = getAlpha();
    glm::vec4 textColor = {_color.red / MAX_COLOR, _color.green / MAX_COLOR, _color.blue / MAX_COLOR, alpha };
    _textRenderer->draw(x, y, _text, textColor);
}

void TextOverlay::setProperties(const QScriptValue& properties) {
    Overlay2D::setProperties(properties);
    
    QScriptValue font = properties.property("font");
    if (font.isObject()) {
        if (font.property("size").isValid()) {
            setFontSize(font.property("size").toInt32());
        }
    }

    QScriptValue text = properties.property("text");
    if (text.isValid()) {
        setText(text.toVariant().toString());
    }

    QScriptValue backgroundColor = properties.property("backgroundColor");
    if (backgroundColor.isValid()) {
        QScriptValue red = backgroundColor.property("red");
        QScriptValue green = backgroundColor.property("green");
        QScriptValue blue = backgroundColor.property("blue");
        if (red.isValid() && green.isValid() && blue.isValid()) {
            _backgroundColor.red = red.toVariant().toInt();
            _backgroundColor.green = green.toVariant().toInt();
            _backgroundColor.blue = blue.toVariant().toInt();
        }
    }

    if (properties.property("backgroundAlpha").isValid()) {
        _backgroundAlpha = properties.property("backgroundAlpha").toVariant().toFloat();
    }

    if (properties.property("leftMargin").isValid()) {
        setLeftMargin(properties.property("leftMargin").toVariant().toInt());
    }

    if (properties.property("topMargin").isValid()) {
        setTopMargin(properties.property("topMargin").toVariant().toInt());
    }
}

TextOverlay* TextOverlay::createClone() const {
    return new TextOverlay(this);
}

QScriptValue TextOverlay::getProperty(const QString& property) {
    if (property == "font") {
        QScriptValue font = _scriptEngine->newObject();
        font.setProperty("size", _fontSize);
        return font;
    }
    if (property == "text") {
        return _text;
    }
    if (property == "backgroundColor") {
        return xColorToScriptValue(_scriptEngine, _backgroundColor);
    }
    if (property == "backgroundAlpha") {
        return _backgroundAlpha;
    }
    if (property == "leftMargin") {
        return _leftMargin;
    }
    if (property == "topMargin") {
        return _topMargin;
    }

    return Overlay2D::getProperty(property);
}

QSizeF TextOverlay::textSize(const QString& text) const {
    auto extents = _textRenderer->computeExtent(text);

    return QSizeF(extents.x, extents.y);
}

void TextOverlay::setFontSize(int fontSize) {
    _fontSize = fontSize;

    auto oldTextRenderer = _textRenderer;
    _textRenderer = TextRenderer::getInstance(SANS_FONT_FAMILY, _fontSize, DEFAULT_FONT_WEIGHT);
    delete oldTextRenderer;
}
