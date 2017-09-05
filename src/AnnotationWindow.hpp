#ifndef sonarlog_annotation_AnnotationWindow_hpp
#define sonarlog_annotation_AnnotationWindow_hpp

#include <iostream>
#include <QtGui>
#include <base/samples/Sonar.hpp>
#include <sonar_processing/SonarHolder.hpp>
#include <image_picker_tool/ImagePickerTool.hpp>

#define APP_NAME "Sonarlog Annotation Tool"

namespace sonarlog_annotation {

class LoadSonarLogWorker;
class AnnotationWindow;

class LoadSonarLogWorker : public QObject {
    Q_OBJECT
public:
    LoadSonarLogWorker(AnnotationWindow* annotation_window)
        : annotation_window_(annotation_window) {
    }

    virtual ~LoadSonarLogWorker() {
    }

    void requestLoadSonarLog() {
        emit loadSonarLogRequested();
    }

signals:
    void loadSonarLogRequested();
    void finished();

public slots:
    void performLoadSonarLog();

private:

    AnnotationWindow* annotation_window_;
};


class AnnotationWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit AnnotationWindow(QWidget *parent = 0);
    void loadSonarLog();

protected:
    bool eventFilter(QObject* obj, QEvent* event);

protected slots:
    void currentItemChanged(QTreeWidgetItem * current, QTreeWidgetItem * previous);
    void pathAppended(QList<QPointF>& path, QVariant& user_data);
    void pointChanged(const QList<QPointF>& path, const QVariant& user_data, QBool& ignore);
    void pointAppened(const QPointF& point, QBool& ignore);
    void openLogFileClicked(bool checked);
    void enableEnhancementStateChanged(int state);
    void enablePreprocessingStateChanged(int state);
    void loadLogFileFinished();

signals:
    void performLoadSonarLogFile();

private:

    typedef QMap<QString, QList<QPointF> > AnnotationMap;

    void setupLoadSonarLogWorker();
    void setupImagePickerTool();
    void setupRightDockWidget();
    void setupTreeView();

    void loadSamples(const QString& logfilepath, QList<base::samples::Sonar>& samples);
    void loadSonarImage(int sample_number, bool redraw = false);
    void loadTreeItems(const QList<base::samples::Sonar>& samples);
    void loadAnnotationTreeItems(const QList<AnnotationMap>& annotations);
    void loadAnnotations(int index);

    void previousSample();
    void nextSample();

    QTreeWidgetItem* createSampleTreeItem(const base::samples::Sonar& sample, QTreeWidgetItem* parent = NULL);
    QTreeWidgetItem* createItem(const QString& name, const QString& value, QTreeWidgetItem* parent = NULL);

    bool processImagePickerToolKeyPress(QKeyEvent* event);
    bool processImagePickerToolKeyRelease(QKeyEvent* event);
    bool processTreeWidgetKeyRelease(QKeyEvent* event);

    void saveAnnotation(QString annotation_name, const QList<QPointF>& points);
    void updateAnnotation(QString annotation_name, const QList<QPointF>& points);
    void addAnnotationTreeItem(int index, const QString& annotation_name, const QList<QPointF>& points);

    void releaseAnnotations();
    void releaseTreeItems();

    void readAnnotationFile();
    void writeAnnotationFile();

    QString generateAnnotationFilePath(const QString& logfilepath);

    std::vector<cv::Point2f> toCvPoints(const QList<QPointF>& points);
    QList<QPointF> toQtPoints(const std::vector<cv::Point2f>& points);


    QTreeWidgetItem* createAnnotationItem(const QString& name, const QList<QPointF>& points);
    QPushButton *open_logfile_button_;
    QCheckBox *enable_enhancement_button_;
    QCheckBox *enable_preprocessing_button_;
    QTreeWidget *treewidget_;
    image_picker_tool::ImagePickerTool* image_picker_tool_;


    QList<base::samples::Sonar> samples_;
    QList<AnnotationMap> annotations_;
    QList<QTreeWidgetItem*> treeitems_;
    QList<QTreeWidgetItem*> annotation_treeitems_;
    sonar_processing::SonarHolder sonar_holder_;

    QProgressDialog *load_sonarlog_progress_;

    int current_index_;
    QString current_annotation_name_;

    QString logfilepath_;

    QThread thread_;
    LoadSonarLogWorker load_sonarlog_worker_;

    QString annotation_filepath_;
    QString last_annotation_name_;
};

} /* namespace sonarlog_annotation */


#endif /* AnnotationWindow_hpp */
