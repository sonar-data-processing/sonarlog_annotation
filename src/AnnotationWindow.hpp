#ifndef AnnotationWindow_hpp
#define AnnotationWindow_hpp

#include <iostream>
#include <QtGui>
#include <base/samples/Sonar.hpp>
#include "sonar_processing/SonarHolder.hpp"
#include "image_picker_tool/ImagePickerTool.hpp"

namespace sonarlog_annotation {

class AnnotationWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit AnnotationWindow(QWidget *parent = 0);

protected:
    bool eventFilter(QObject* obj, QEvent* event);

protected slots:
    void currentItemChanged(QTreeWidgetItem * current, QTreeWidgetItem * previous);
    void pathAppended(image_picker_tool::PathElement& path_element);
    void pointChanged(image_picker_tool::PathElement& path_element);

private:

    typedef QMap<QString, QList<QPointF> > AnnotationMap;

    void setupImagePickerTool();
    void setupRightDockWidget();
    void setupTreeView();

    void loadSamples(const std::string& logfilepath, QList<base::samples::Sonar>& samples);
    void loadSonarImage(int sample_number);
    void loadTreeViewItems(const QList<base::samples::Sonar>& samples);

    void previousSample();
    void nextSample();

    void loadAnnotations(int index);

    QTreeWidgetItem* createSampleTreeItem(const base::samples::Sonar& sample, QTreeWidgetItem* parent = NULL);
    QTreeWidgetItem* createItem(const QString& name, const QString& value, QTreeWidgetItem* parent = NULL);

    bool processImagePickerToolKeyPress(QKeyEvent* event);
    bool processImagePickerToolKeyRelease(QKeyEvent* event);

    void saveAnnotation(QString annotation_name, const QList<QPointF>& points);

    QTreeWidgetItem* createAnnotationItem(const QString& name, const QList<QPointF>& points);

    image_picker_tool::ImagePickerTool* image_picker_tool_;

    QPushButton *open_logfile_button_;
    QTreeWidget *treewidget_;
    QList<base::samples::Sonar> samples_;
    QList<AnnotationMap*> annotations_;
    QList<QTreeWidgetItem*> treeitems_;
    QList<QTreeWidgetItem*> annotation_treeitems_;
    sonar_processing::SonarHolder sonar_holder_;

    int current_index_;
};

} /* namespace sonarlog_annotation */


#endif /* AnnotationWindow_hpp */
