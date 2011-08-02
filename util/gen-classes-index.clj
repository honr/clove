#!/usr/bin/env clove
;; | clojure

(ns bin.gen-classes-index
  (:import [java.io File FilenameFilter]))

;; several parts taken from swank-clojure (Eclipse Public License - v 1.0).

(def classes-index-file
  (str *home* "/.local/share/java/classes.index"))

(defn jar-file? [#^String n]        (.endsWith n ".jar"))
(defn clj-file? [#^String n]        (.endsWith n ".clj"))
(defn class-file? [#^String n]      (.endsWith n ".class"))
(defn clojure-ns-file? [#^String n] (.endsWith n "__init.class"))
(defn named-class? [x] (re-find #"^[^\$]+(\$[^\d]\w*)*\.class" (:file x)))
(defn clojure-fn? [x] (re-find #"\$.*__\d+\.class" (:file x)))

(defn unmunge
  "Converts a javafied name to a clojure symbol name"
  ([#^String name]
     (reduce (fn [#^String s [to from]]
               (.replaceAll s from (str to)))
             name
             (dissoc clojure.lang.Compiler/CHAR_MAP \-))))

(defn class-or-ns-name
  "Returns the Java class or Clojure namespace name for a class relative path."
  [#^String n]
  (cond
   (clj-file? n)
   (-> n
       (.replace ".clj" "")
       (.replace File/separator ".")
       (unmunge)
       (.replace "_" "-"))

   (clojure-ns-file? n)
   (-> n
       (.replace "__init.class" "")
       (.replace File/separator ".")
       (unmunge)
       (.replace "_" "-"))

   (class-file? n)
   (-> n
       (.replace ".class" "")
       (.replace File/separator "."))))

(defn classes-on-path [^File f ^File loc]
  "Returns a list of classes found on the specified path location
  (jar or directory), each comprised of a map with the following keys:
    :name  Java class or Clojure namespace name
    :loc   Classpath entry (directory or jar) on which the class is located
    :file  Path of the class file, relative to :loc"

  (if (.isDirectory f)
    (mapcat #(classes-on-path % loc)
            (.listFiles f (proxy [FilenameFilter] []
                            (accept [f n] (not (jar-file? n))))))
    (let [f-name (.getName f)]
      (cond
       (jar-file? f-name)
       (try
         (map (fn [fp]
                {:loc (.getPath loc) :file fp :name (class-or-ns-name fp)})
              (filter #(or (class-file? %)
                           (clj-file? %))
                      (map #(.getName #^java.util.jar.JarEntry %)
                           (enumeration-seq (.entries (java.util.jar.JarFile. f))))))
         (catch Exception e []))

       (or (clj-file? f-name)
           (class-file? f-name))
       (let [fp (.getPath f), lp (.getPath loc)
             m (re-matcher (re-pattern (java.util.regex.Pattern/quote
                                        (str lp File/separator))) fp)]
         (when (.find m) ; must be descendent of loc
           (let [fpr (.substring fp (.end m))]
             [{:loc lp :file fpr :name (class-or-ns-name fpr)}])))
       
       :else
       nil))))

(defn classpath-entries []
  (mapcat
   (fn [cp]
     (mapcat

      (fn [path] ;; expand "*" (jdk 1.6+)
        (let [f (java.io.File. path)]
          (if (= (.getName f) "*")
            (.list (.getParentFile f)
                   (proxy [FilenameFilter] []
                     (accept [d n] (jar-file? n))))
            [f])))
      
      (enumeration-seq
       (java.util.StringTokenizer.
        cp File/pathSeparator))))
   
   (filter identity
           [(System/getProperty "sun.boot.class.path")
            (System/getProperty "java.ext.dirs")
            (System/getProperty "java.class.path")])))

(defn main []
  (with-open [out-stream
              (java.io.PrintWriter.
               (java.io.FileOutputStream.
                classes-index-file) true)]
    (doseq [c (filter #(and (not (clojure-fn? %))
                            (or (named-class? %)
                                (and (clj-file? (:file %))
                                     (not (= (:file %) "project.clj")))))
                      
                      (mapcat #(classes-on-path % %)
                              (classpath-entries)))]
      (.println out-stream (:name c)))))

(main)
