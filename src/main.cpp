#include <iostream>
#include "AnnotationWindow.hpp"

using namespace sonarlog_annotation;

int main(int argc, char **argv) {
    std::cout << "Sonar log Annotation" << std::endl;
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(APP_NAME);


    AnnotationWindow *annotation_window = new AnnotationWindow();
    if (QCoreApplication::arguments().size() > 1) {
        annotation_window->setStreamName(QCoreApplication::arguments().at(1));
    }
    else {
        annotation_window->setStreamName("gemini.sonar_samples");
    }

    annotation_window->setWindowTitle(QString(APP_NAME));
    annotation_window->show();
    return app.exec();

    return 0;
}
