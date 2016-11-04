#include <iostream>
#include "AnnotationWindow.hpp"

using namespace sonarlog_annotation;

#define APP_NAME    "Sonarlog Annotation Tool"

int main(int argc, char **argv) {
    std::cout << "Sonar log Annotation" << std::endl;
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(APP_NAME);
    AnnotationWindow *annotation_window = new AnnotationWindow();
    annotation_window->setWindowTitle(QString(APP_NAME));
    annotation_window->show();
    return app.exec();
}
