#include <opencv2/opencv.hpp>
#include "AnnotationFileReader.hpp"
#include "rock_util/LogReader.hpp"
#include "rock_util/Utilities.hpp"
#include "AnnotationWindow.hpp"

#define DATA_PATH "/home/gustavoneves/masters_degree/dev/sonar_toolkit/data"

namespace sonarlog_annotation {

void LoadSonarLogWorker::performLoadSonarLog() {
    annotation_window_->loadSonarLog();
    emit finished();
}


AnnotationWindow::AnnotationWindow(QWidget *parent)
    : current_index_(-1)
    , current_annotation_name_("")
    , load_sonarlog_worker_(this)
    , load_sonarlog_progress_(NULL)
    , last_annotation_name_("")
{
    setupLoadSonarLogWorker();
    setupTreeView();
    setupRightDockWidget();
    setupImagePickerTool();
}

void AnnotationWindow::setupLoadSonarLogWorker() {
    load_sonarlog_worker_.moveToThread(&thread_);
    connect(&thread_, SIGNAL(started()), &load_sonarlog_worker_, SLOT(performLoadSonarLog()));
    connect(&load_sonarlog_worker_, SIGNAL(loadSonarLogRequested()), &thread_, SLOT(start()));
    connect(&load_sonarlog_worker_, SIGNAL(finished()), &thread_, SLOT(quit()), Qt::DirectConnection);
    connect(&load_sonarlog_worker_, SIGNAL(finished()), this, SLOT(loadLogFileFinished()));
}

void AnnotationWindow::setupImagePickerTool() {
    image_picker_tool_ = new image_picker_tool::ImagePickerTool();
    image_picker_tool_->installEventFilter(this);
    connect(image_picker_tool_, SIGNAL(pathAppended(QList<QPointF>&, QVariant&)), this, SLOT(pathAppended(QList<QPointF>&, QVariant&)));
    connect(image_picker_tool_, SIGNAL(pointChanged(const QList<QPointF>&, const QVariant&, QBool&)), this, SLOT(pointChanged(const QList<QPointF>&, const QVariant&, QBool&)));
    connect(image_picker_tool_, SIGNAL(pointAppened(const QPointF&, QBool&)), this, SLOT(pointAppened(const QPointF&, QBool&)));
    setCentralWidget(image_picker_tool_);
}

void AnnotationWindow::setupRightDockWidget() {
    QDockWidget *dock = new QDockWidget(this);
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    QVBoxLayout *layout = new QVBoxLayout();
    open_logfile_button_ = new QPushButton("Open Sonar Log");

    QFrame *frame = new QFrame();
    layout->addWidget(open_logfile_button_);
    layout->addWidget(treewidget_);

    frame->setLayout(layout);
    dock->setWidget(frame);

    connect(open_logfile_button_, SIGNAL(clicked(bool)), this, SLOT(openLogFileClicked(bool)));

    setTabOrder(treewidget_, open_logfile_button_);
    addDockWidget(Qt::LeftDockWidgetArea, dock);
}

void AnnotationWindow::setupTreeView() {
    treewidget_ = new QTreeWidget();
    treewidget_->setHeaderItem(createItem("Name", "Value"));
    treewidget_->setFocus();
    treewidget_->installEventFilter(this);
    connect(treewidget_, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
            this, SLOT(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)));
}

void AnnotationWindow::loadSamples(const QString& logfilepath, QList<base::samples::Sonar>& samples) {

    rock_util::LogReader reader(logfilepath.toStdString());
    rock_util::LogStream stream = reader.stream("gemini.sonar_samples");

    do {
        base::samples::Sonar sample;
        stream.next<base::samples::Sonar>(sample);
        samples_ << sample;
        annotations_.append(AnnotationMap());
    } while(stream.current_sample_index() < stream.total_samples());
}

void AnnotationWindow::loadSonarImage(int sample_number) {
    if (samples_.count() > 0 &&
        sample_number < samples_.count() &&
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

void AnnotationWindow::loadTreeItems(const QList<base::samples::Sonar>& samples) {

    if (samples.count()) {
        QList<base::samples::Sonar>::const_iterator it;
        for (it = samples.begin(); it != samples.end(); it++) {
            treeitems_.append(createSampleTreeItem(*it, NULL));
            annotation_treeitems_.append(NULL);
        }

        loadAnnotationTreeItems(annotations_);
        treewidget_->insertTopLevelItems(0, treeitems_);
        treewidget_->setCurrentItem(treeitems_[(current_index_ == -1) ? 0 : current_index_]);
    }
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
    int isTopLevelItem = true;

    while (item != NULL && (index = treewidget_->indexOfTopLevelItem(item)) == -1){
        item = item->parent();
        isTopLevelItem = false;
    }
    qDebug() << "index: " << index;
    qDebug() << "current_index_: " << current_index_;

    if (index != -1 && current_index_ != -1 && index != current_index_) {
        loadSonarImage(index);
        loadAnnotations(index);
    }

    if (!isTopLevelItem && !current->data(0, Qt::UserRole).isNull()) {
        current_annotation_name_ = current->data(0, Qt::UserRole).toString();
        qDebug() << "Selected annotation: " << current_annotation_name_;
        if (annotations_[current_index_].contains(current_annotation_name_)) {
            int index = image_picker_tool_->findIndexByUserData(current_annotation_name_);
            qDebug() << "Path index: " << index;
            image_picker_tool_->setSelected(index);
            return;
        }
    }

    current_annotation_name_ = "";
    image_picker_tool_->setSelected(-1);
}

bool AnnotationWindow::eventFilter(QObject* obj, QEvent* event) {

    if (obj == image_picker_tool_) {
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
    }
    else if (obj == treewidget_) {
        if (event->type() == QEvent::KeyRelease) {
            QKeyEvent* key = static_cast<QKeyEvent*>(event);
            if (processTreeWidgetKeyRelease(key)) {
                return true;
            }
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

bool AnnotationWindow::processTreeWidgetKeyRelease(QKeyEvent* event) {
    switch(event->key()){
        case Qt::Key_Delete: {
            if (!current_annotation_name_.isEmpty()) {

                QMessageBox::StandardButton reply = QMessageBox::question(this,
                                                        "Delete annotation",
                                                        QString("Do you wnat delete this annotation: %1").
                                                            arg(current_annotation_name_),
                                                        QMessageBox::Yes | QMessageBox::No);
                if (reply == QMessageBox::Yes) {
                    qDebug() << "Delete the annotation: " << current_annotation_name_;

                    if (annotations_[current_index_].contains(current_annotation_name_)) {

                        int index = image_picker_tool_->findIndexByUserData(current_annotation_name_);
                        qDebug() << "Path Index: " << index;

                        //remove path from image_picker_tool
                        image_picker_tool_->removePath(index);

                        //remove from annotations map
                        annotations_[current_index_].remove(current_annotation_name_);

                        //write new annotation file
                        writeAnnotationFile();

                        QString annotation_name = current_annotation_name_;
                        current_annotation_name_ = "";

                        //remove from treewidget
                        for (size_t i = 0; i < annotation_treeitems_[current_index_]->childCount(); i++) {
                            QTreeWidgetItem *item = annotation_treeitems_[current_index_]->child(index);
                            if (item->text(0) == annotation_name) {
                                annotation_treeitems_[current_index_]->removeChild(item);
                                delete item;
                                break;
                            }
                        }

                        if (annotation_treeitems_[current_index_]->childCount() == 0) {
                            treeitems_[current_index_]->removeChild(annotation_treeitems_[current_index_]);
                            delete annotation_treeitems_[current_index_];
                            annotation_treeitems_[current_index_] = NULL;
                        }
                    }
                }
            }
            return true;
        }
    }
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

void AnnotationWindow::pathAppended(QList<QPointF>& path, QVariant& user_data) {
    QInputDialog *input_dialog = new QInputDialog(this);

    if (current_index_ != -1) {
        int total_annotations = annotations_[current_index_].size();
        QString annotation_name;

        if (last_annotation_name_.isEmpty()) {
            annotation_name = QString("annotation%1").arg(total_annotations);
        }
        else {
            annotation_name = last_annotation_name_;
        }

        bool ok = false;
        annotation_name =  input_dialog->getText(this, "Annotation", "Annotation Name:", QLineEdit::Normal, annotation_name, &ok);

        if (!ok) {
            image_picker_tool_->removeLastPath();
            return;
        }

        qDebug() << "annotation_name: " << annotation_name;
        if (annotations_[current_index_].contains(annotation_name)){
            QString message = QString("It already exist an annotation with name: %1").arg(annotation_name);
            QMessageBox messagebox(QMessageBox::Information, "Annotation name is invalid ", message);
            messagebox.exec();
            image_picker_tool_->removeLastPath();
            return;
        }

        saveAnnotation(annotation_name, path);
        last_annotation_name_ = annotation_name;
        user_data = annotation_name;
    }
}

void AnnotationWindow::pointChanged(const QList<QPointF>& path, const QVariant& user_data, QBool& ignore) {

    for (int i = 0; i < path.size(); i++) {
        if (sonar_holder_.cart_to_polar_index(path.at(i).x(), path.at(i).y()) == -1) {
            ignore = QBool(true);
            return;
        }
    }

    updateAnnotation(user_data.toString(), path);
}

void AnnotationWindow::pointAppened(const QPointF& point, QBool& ignore) {
    ignore = QBool((sonar_holder_.cart_to_polar_index((int)point.x(), (int)point.y()) == -1));
}

void AnnotationWindow::saveAnnotation(QString annotation_name, const QList<QPointF>& points) {
    annotations_[current_index_].insert(annotation_name, points);
    addAnnotationTreeItem(current_index_, annotation_name, points);
    writeAnnotationFile();
}

void AnnotationWindow::addAnnotationTreeItem(int index, const QString& annotation_name, const QList<QPointF>& points) {

    if (!annotation_treeitems_[index]) {
        QTreeWidgetItem *item = createItem("Annotations", "", treeitems_[index]);
        annotation_treeitems_[index] = item;
        treeitems_[index]->addChild(item);
    }

    QTreeWidgetItem* annotation_item = createAnnotationItem(annotation_name, points);
    annotation_item->setData(0, Qt::UserRole, annotation_name);
    annotation_item->setData(1, Qt::UserRole, index);
    annotation_treeitems_[index]->addChild(annotation_item);
}

void AnnotationWindow::loadAnnotationTreeItems(const QList<AnnotationMap>& annotations) {
    for (size_t sample_index = 0; sample_index < annotations.size(); sample_index++) {
        AnnotationMap::const_iterator it = annotations[sample_index].begin();
        while (it != annotations[sample_index].end()) {
            addAnnotationTreeItem(sample_index, it.key(), it.value());
            it++;
        }
    }
}

void AnnotationWindow::updateAnnotation(QString annotation_name, const QList<QPointF>& points) {
    if (annotations_[current_index_].contains(annotation_name)) {
        annotations_[current_index_].insert(annotation_name, points);

        writeAnnotationFile();

        for (size_t index = 0; index < annotation_treeitems_[current_index_]->childCount(); index++) {
            QTreeWidgetItem *item = annotation_treeitems_[current_index_]->child(index);
            if (item->text(0) == annotation_name) {
                qDebug() << "The annotation is: " << annotation_name;

                if (item->childCount() != points.size()) {
                    qDebug() << "Point and Child don't have the same time";
                    return;
                }

                QString point_string_list;
                for (size_t idx = 0; idx < item->childCount() && idx < points.size(); idx++) {
                    QString point_string = QString("(%1,%2)").
                                               arg(points[idx].x()).
                                               arg(points[idx].y());

                    QTreeWidgetItem *child = item->child(idx);
                    child->setText(0, QString("%1").arg(idx));
                    child->setText(1, point_string);

                    point_string_list += point_string;

                    if (idx < points.size()-1) {
                        point_string_list += ", ";
                    }
                }

                item->setText(1, point_string_list);

                return;
            }
        }
    }
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
    image_picker_tool_->setSelected(-1);

    if (!annotations_[index].isEmpty()) {
        AnnotationMap::iterator it;
        QList<QVariant> user_data_list;
        QList<QList<QPointF> > path_list;

        for (it = annotations_[index].begin();
              it != annotations_[index].end();
              it++) {
            user_data_list << it.key();
            path_list << it.value();
        }
        image_picker_tool_->appendPaths(path_list, user_data_list);
    }
}

void AnnotationWindow::releaseAnnotations() {
    annotations_.clear();
}

void AnnotationWindow::releaseTreeItems() {
    treewidget_->clear();
    treeitems_.clear();
    annotation_treeitems_.clear();
}

void AnnotationWindow::openLogFileClicked(bool checked) {
    logfilepath_ = QFileDialog::getOpenFileName(this, "Open Sonar Log File", "", "PocoLog (*.log)");

    if (!logfilepath_.isEmpty()) {
        setWindowTitle(QString("%1-%2").arg(APP_NAME).arg(logfilepath_));
        annotation_filepath_ = generateAnnotationFilePath(logfilepath_);

        current_index_ = -1;
        samples_.clear();
        releaseAnnotations();
        releaseTreeItems();

        QString message = QString("Loading log file:\n%1").arg(logfilepath_);
        load_sonarlog_progress_ = new QProgressDialog(message, NULL, 0, 0, this);
        load_sonarlog_progress_->setWindowModality(Qt::WindowModal);
        load_sonarlog_progress_->show();

        load_sonarlog_worker_.requestLoadSonarLog();
    }
}

void AnnotationWindow::loadSonarLog() {
    current_index_ = -1;
    loadSamples(logfilepath_, samples_);
    readAnnotationFile();
    loadSonarImage(0);
    loadAnnotations(0);
}

void AnnotationWindow::loadLogFileFinished() {
    loadTreeItems(samples_);
    load_sonarlog_progress_->hide();
    delete load_sonarlog_progress_;
    load_sonarlog_progress_ = NULL;
}

QString AnnotationWindow::generateAnnotationFilePath(const QString& logfilepath) {
    QFileInfo file_info(logfilepath);
    return file_info.absolutePath() + "/" +  file_info.completeBaseName() + "_annotation.yml";
}

void AnnotationWindow::readAnnotationFile() {
    QFileInfo info(annotation_filepath_);

    if (info.exists() && info.isFile()) {
        AnnotationFileReader reader(annotation_filepath_.toStdString());
        std::vector<AnnotationFileReader::AnnotationMap> samples_annotations = reader.read();

        for (size_t sample_idx = 0; sample_idx < samples_annotations.size(); sample_idx++) {
            AnnotationFileReader::AnnotationMap annotations = samples_annotations[sample_idx];
            AnnotationFileReader::AnnotationMap::iterator annotation_it = annotations.begin();

            while (annotation_it != annotations.end()) {
                QString annotation_name = QString::fromStdString(annotation_it->first);
                QList<QPointF> points = toQtPoints(annotation_it->second);
                annotations_[sample_idx].insert(annotation_name, points);
                annotation_it++;
            }
        }

    }
}

void AnnotationWindow::writeAnnotationFile() {
    cv::FileStorage file_storage(annotation_filepath_.toStdString(), cv::FileStorage::WRITE | cv::FileStorage::FORMAT_YAML);

    int sample_number = 0;
    for (int sample_number = 0; sample_number < annotations_.count(); sample_number++) {
        file_storage << QString("sample_%1").arg(sample_number).toStdString();
        file_storage << "{";
        if (!annotations_[sample_number].isEmpty()) {
            AnnotationMap::iterator it;
            for (it = annotations_[sample_number].begin(); it != annotations_[sample_number].end(); it++) {
                file_storage << it.key().toStdString() << cv::Mat(toCvPoints(it.value()));
            }
        }
        file_storage << "}";
    }
    file_storage.release();
}

std::vector<cv::Point2f> AnnotationWindow::toCvPoints(const QList<QPointF>& points) {
    std::vector<cv::Point2f> cv_points(points.count());
    for (size_t i = 0; i < points.count(); i++) {
        cv_points[i] = cv::Point2f(points[i].x(), points[i].y());
    }
    return cv_points;
}

QList<QPointF> AnnotationWindow::toQtPoints(const std::vector<cv::Point2f>& points) {
    QList<QPointF> qpoints;
    for (size_t i = 0; i < points.size(); i++) {
        qpoints.append(QPointF(points[i].x, points[i].y));
    }
    return qpoints;

}


} /* namespace sonarlog_annotation */
