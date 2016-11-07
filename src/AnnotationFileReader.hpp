#ifndef sonarlog_annotation_AnnotationFileReader_hpp
#define sonarlog_annotation_AnnotationFileReader_hpp

#include <map>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>

namespace sonarlog_annotation {


class AnnotationFileReader {
public:

    typedef std::map<std::string, std::vector<cv::Point2f> > AnnotationMap;

    AnnotationFileReader(const std::string& filepath) 
        : filepath_(filepath)
    {
    }

    virtual ~AnnotationFileReader() {
    }
    
    std::vector<AnnotationFileReader::AnnotationMap> read();

private:

    void readAnnotation(cv::FileNode node, AnnotationFileReader::AnnotationMap& annotations);

    std::string filepath_;

};

} /* namespace sonarlog_annotation */

#endif /* sonarlog_annotation_AnnotationFileReader_hpp */
