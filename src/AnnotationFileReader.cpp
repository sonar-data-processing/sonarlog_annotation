#include <stdio.h>
#include "sonar_processing/ImageUtil.hpp"
#include "sonarlog_annotation/AnnotationFileReader.hpp"

namespace sonarlog_annotation {

std::vector<AnnotationFileReader::AnnotationMap> AnnotationFileReader::read() {
    cv::FileStorage file_storage(filepath_, cv::FileStorage::READ);

    cv::FileNode node = file_storage.root();
    cv::FileNodeIterator it = node.begin();

    std::vector<AnnotationMap> samples_annotations;

    while (it != node.end()) {
        AnnotationMap annotation_map;
        readAnnotation(*it, annotation_map);
        samples_annotations.push_back(annotation_map);
        it++;
    }

    return samples_annotations;
}

void AnnotationFileReader::readAnnotation(cv::FileNode node, AnnotationFileReader::AnnotationMap& annotations) {
    cv::FileNodeIterator it = node.begin();
    cv::Mat mat;

    while (it != node.end()) {
        (*it) >> mat;
        std::vector<cv::Point2f> points = sonar_processing::image_util::mat2vector<cv::Point2f>(mat);
        annotations.insert(std::make_pair((*it).name(), points));
        it++;
    }
}

} /* sonarlog_annotation */
