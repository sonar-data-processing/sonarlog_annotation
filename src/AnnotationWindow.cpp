#include <opencv2/opencv.hpp>
#include "rock_util/LogReader.hpp"
#include "rock_util/Utilities.hpp"
#include "AnnotationWindow.hpp"

#define DATA_PATH "/home/gustavoneves/masters_degree/dev/sonar_toolkit/data"

namespace sonarlog_annotation {

AnnotationWindow::AnnotationWindow(QWidget *parent)
    : current_index_(-1)
{
    const std::string logfilepath = std::string(DATA_PATH) + "/logs/gemini-jequitaia.4.log";
    loadSamples(logfilepath, samples_);
    setupRightDockWidget();
    setupImagePickerTool();
    loadSonarImage(0);
}

void AnnotationWindow::setupImagePickerTool() {
    image_picker_tool_ = new image_picker_tool::ImagePickerTool();
    image_picker_tool_->installEventFilter(this);
    connect(image_picker_tool_, SIGNAL(pathAppended(image_picker_tool::PathElement&)), this, SLOT(pathAppended(image_picker_tool::PathElement&)));
    connect(image_picker_tool_, SIGNAL(pointChanged(image_picker_tool::PathElement&)), this, SLOT(pointChanged(image_picker_tool::PathElement&)));
    setCentralWidget(image_picker_tool_);
}

void AnnotationWindow::setupRightDockWidget() {
    setupTreeView();

    QDockWidget *dock = new QDockWidget(this);
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    QVBoxLayout *layout = new QVBoxLayout();
    open_logfile_button_ = new QPushButton("Open Sonar Log");

    QFrame *frame = new QFrame();
    layout->addWidget(open_logfile_button_);
    layout->addWidget(treewidget_);

    frame->setLayout(layout);
    dock->setWidget(frame);

    setTabOrder(treewidget_, open_logfile_button_);
    addDockWidget(Qt::LeftDockWidgetArea, dock);
}

void AnnotationWindow::setupTreeView() {
    treewidget_ = new QTreeWidget();
    loadTreeViewItems(samples_);
    treewidget_->setFocus();
    connect(treewidget_, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
            this, SLOT(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)));
}

void AnnotationWindow::loadSamples(const std::string& logfilepath, QList<base::samples::Sonar>& samples) {

    samples.clear();
    qDeleteAll(annotations_);
    annotations_.clear();

    rock_util::LogReader reader(logfilepath);
    rock_util::LogStream stream = reader.stream("gemini.sonar_samples");

    do {
        base::samples::Sonar sample;
        stream.next<base::samples::Sonar>(sample);
        samples_ << sample;
        annotations_.append(new AnnotationMap());
    } while(stream.current_sample_index() < stream.total_samples());

    annotations_.reserve(samples_.size());
}

void AnnotationWindow::loadSonarImage(int sample_number) {
    if (samples_.count() > 0 &&
        sample_number < samples_.count()-1 &&
        sample_number != current_index_) {

        base::samples::Sonar sample = samples_.value(sample_number);
        sonar_holder_.Reset(sample.bins,
                            rock_util::Utilities::get_radians(sample.bearings),
                            sample.beam_width.getRad(),
                            sample.bin_count,
                            sample.beam_count);

        cv::Mat cart_image = sonar_holder_.cart_image();
        cart_image.convertTo(cart_image, CV_8U, 255.0);
        cv::cvtColor(cart_image, cart_image, CV_GRAY2BGR);
        image_picker_tool_->loadImage(cart_image);
        current_index_ = sample_number;
    }
}

void AnnotationWindow::loadTreeViewItems(const QList<base::samples::Sonar>& samples) {
    treewidget_->setHeaderItem(createItem("Name", "Value"));

    QList<base::samples::Sonar>::const_iterator it;
    for (it = samples.begin(); it != samples.end(); it++) {
        treeitems_.append(createSampleTreeItem(*it, NULL));
        annotation_treeitems_.append(NULL);
    }

    treewidget_->insertTopLevelItems(0, treeitems_);
    treewidget_->setCurrentItem(treeitems_[0]);
}

QTreeWidgetItem* AnnotationWindow::createSampleTreeItem(const base::samples::Sonar& sample, QTreeWidgetItem* parent) {

    int num = samples_.count();
    int number_of_digits = 1;
    while (num /= 10) number_of_digits++;

    QString name = QString("Sample#%1").arg(treeitems_.count()+1, number_of_digits, 10, QChar('0'));
    QString value = QString::fromStdString((sample.time.toString()));
    QTreeWidgetItem *item = createItem(name, value);
    item->addChild(createItem(QString("BinCount"), QString("%1").arg(sample.bin_count), item));
    item->addChild(createItem(QString("BeamCount"), QString("%1").arg(sample.beam_count), item));
    int image_width = cos(sample.beam_width.rad - M_PI_2) * sample.bin_count * 2.0;
    item->addChild(createItem(QString("ImageWidth"), QString("%1").arg(image_width), item));
    return item;
}

QTreeWidgetItem* AnnotationWindow::createItem(const QString& name, const QString& value, QTreeWidgetItem* parent) {
    QStringList strings;
    strings << name << value;
    return new QTreeWidgetItem(parent, strings);
}

void AnnotationWindow::currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous) {
    int index;
    QTreeWidgetItem *item = current;

    while (item != NULL && (index = treewidget_->indexOfTopLevelItem(item)) == -1){
        item = item->parent();
    }

    if (index != -1) {
        loadSonarImage(index);
        loadAnnotations(index);
    }
}

bool AnnotationWindow::eventFilter(QObject* obj, QEvent* event) {

    if (event->type()==QEvent::KeyPress) {
        QKeyEvent* key = static_cast<QKeyEvent*>(event);
        if (processImagePickerToolKeyPress(key)) {
            return true;
        }
    }
    else if (event->type() == QEvent::KeyRelease) {
        QKeyEvent* key = static_cast<QKeyEvent*>(event);
        if (processImagePickerToolKeyRelease(key)) {
            return true;
        }
    }

    return QObject::eventFilter(obj, event);
}

bool AnnotationWindow::processImagePickerToolKeyPress(QKeyEvent* event) {

    if ((event->key()==Qt::Key_Enter) || (event->key()==Qt::Key_Return) ) {
        image_picker_tool_->saveCurrentPath();
        return true;
    }

    return false;
}

bool AnnotationWindow::processImagePickerToolKeyRelease(QKeyEvent* event) {

    switch(event->key()){
        case Qt::Key_Up: {
            previousSample();
            return true;
        }
        case Qt::Key_Down: {
            nextSample();
            return true;
        }
        case Qt::Key_Escape: {
            image_picker_tool_->removeLastPoint();
            return true;
        }
    }

    return false;
}

void AnnotationWindow::previousSample() {
    if ((current_index_-1) >= 0) {
        treewidget_->setCurrentItem(treeitems_[current_index_-1]);
    }
}

void AnnotationWindow::nextSample() {
    if ((current_index_+1) < treeitems_.size()) {
        treewidget_->setCurrentItem(treeitems_[current_index_+1]);
    }
}

void AnnotationWindow::pathAppended(image_picker_tool::PathElement& elements) {
    QInputDialog *input_dialog = new QInputDialog(this);

    if (current_index_ != -1) {
        int total_annotations = annotations_[current_index_]->size();
        QString annotation_name = QString("annotation%1").arg(total_annotations);

        bool ok = false;
        annotation_name =  input_dialog->getText(this, "Annotation", "Annotation Name:", QLineEdit::Normal, annotation_name, &ok);

        if (!ok) {
            image_picker_tool_->removeLastPath();
            return;
        }

        elements.userData = annotation_name;
        saveAnnotation(annotation_name, elements.path);
    }
}

void AnnotationWindow::pointChanged(image_picker_tool::PathElement& path_element) {
    QString annotation_name = path_element.userData.toString();
    qDebug() << "Annotation name: " << annotation_name;
    AnnotationMap* map = annotations_[current_index_];
    AnnotationMap::iterator it = map->find(annotation_name);
    if (it != map->end()) {
        map->insert(annotation_name, path_element.path);
    }
}

void AnnotationWindow::saveAnnotation(QString annotation_name, const QList<QPointF>& points) {
    qDebug() << "Annotation name: " << annotation_name;

    annotations_[current_index_]->insert(annotation_name, points);

    if (!annotation_treeitems_[current_index_]) {
        QTreeWidgetItem *item = createItem("Annotations", "", treeitems_[current_index_]);
        annotation_treeitems_[current_index_] = item;
        treeitems_[current_index_]->addChild(item);
    }

    annotation_treeitems_[current_index_]->addChild(createAnnotationItem(annotation_name, points));
}

QTreeWidgetItem* AnnotationWindow::createAnnotationItem(const QString& name, const QList<QPointF>& points) {

    QList<QTreeWidgetItem*> point_items;
    QString point_string_list;

    for (size_t i = 0; i < points.size(); i++) {
        QString point_string = QString("(%1,%2)").arg(points[i].x()).arg(points[i].y());
        QStringList data;
        data << QString("%1").arg(i) << point_string;
        point_items << new QTreeWidgetItem(data);
        point_string_list += point_string;
        if (i < points.size()-1) {
            point_string_list += ", ";
        }
    }

    QStringList data;
    data << name << point_string_list;
    QTreeWidgetItem *item = new QTreeWidgetItem(data);
    item->addChildren(point_items);
    return item;
}

void AnnotationWindow::loadAnnotations(int index) {
    image_picker_tool_->clearPaths();
    if (!annotations_[current_index_]->isEmpty()) {
        AnnotationMap::iterator it;
        QList<image_picker_tool::PathElement> paths;
        for (it = annotations_[current_index_]->begin();
              it != annotations_[current_index_]->end();
              it++) {

            paths << image_picker_tool::PathElement(it.value(), it.key());
        }
        image_picker_tool_->appendPaths(paths);
    }
}

} /* namespace sonarlog_annotation */
