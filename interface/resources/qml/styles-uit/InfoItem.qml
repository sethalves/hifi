import QtQuick 2.0
import stylesUit 1.0

InfoItem {
    Component.onCompleted: {
        console.warn("warning: including controls-uit is deprecated! please use 'import stylesUit 1.0' instead");
    }
}
