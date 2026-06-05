set(SOURCES
    main.cpp
    app/Application.cpp
    ui/MainWindow.cpp
    ui/VideoPlayerWidget.cpp
    ui/TimelineWidget.cpp
    ui/SubtitleEditorPanel.cpp
    ui/ConflictResolutionDialog.cpp
    ui/ExportDialog.cpp
    model/SubtitleSegment.cpp
    model/UserEdit.cpp
    model/Project.cpp
    db/DatabaseManager.cpp
    db/Schema.cpp
    core/ConflictDetector.cpp
    core/MajorityVoteMerger.cpp
    core/SubtitleImporter.cpp
    core/SubtitleExporter.cpp
    util/TimeUtil.cpp
)

set(HEADERS
    app/Application.h
    ui/MainWindow.h
    ui/VideoPlayerWidget.h
    ui/TimelineWidget.h
    ui/SubtitleEditorPanel.h
    ui/ConflictResolutionDialog.h
    ui/ExportDialog.h
    model/SubtitleSegment.h
    model/UserEdit.h
    model/Project.h
    db/DatabaseManager.h
    db/Schema.h
    core/ConflictDetector.h
    core/MajorityVoteMerger.h
    core/SubtitleImporter.h
    core/SubtitleExporter.h
    util/TimeUtil.h
)

qt_add_executable(SubtitleCrowdCorrector
    ${SOURCES}
    ${HEADERS}
    ${CMAKE_SOURCE_DIR}/resources/app.qrc
)

target_include_directories(SubtitleCrowdCorrector PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(SubtitleCrowdCorrector PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    Qt6::Sql
)

if(WIN32)
    set_target_properties(SubtitleCrowdCorrector PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()
