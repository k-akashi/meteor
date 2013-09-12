#!/bin/sh

for i in 0.004 0.008 0.012 0.016 0.020
do
    OUTFILE="25-delay-${i}.xml";
    rm ${OUTFILE};
    echo "<qomet_scenario duration=\"10\" step=\"${i}\">" >> ${OUTFILE};
    echo "" >> ${OUTFILE};
    echo "  <node name=\"node_ap\" type=\"access_point\" id=\"1\" x=\"0\" y=\"0\" z=\"0\" Pt=\"20\" internal_delay=\"1\"/>" >> ${OUTFILE};
    echo "  <node name=\"node_r\" type=\"regular\" id=\"2\" connection=\"infrastructure\" adapter=\"orinoco\" x=\"0\" y=\"10\" z=\"0\" Pt=\"20\" internal_delay=\"1\"/>" >> ${OUTFILE};
    echo "" >> ${OUTFILE};
    echo "  <environment name=\"env\" alpha=\"5.6\" sigma=\"0\" W=\"0\" noise_power=\"-100\"/>" >> ${OUTFILE};
    echo "" >> ${OUTFILE};
    echo "  <motion node_name=\"node_r\" speed_x=\"2.5\" speed_y=\"0\" speed_z=\"0\" start_time=\"0\" stop_time=\"10\"/>" >> ${OUTFILE};
    echo "" >> ${OUTFILE};
    echo "  <connection from_node=\"node_ap\" to_node=\"node_r\" through_environment=\"env\" standard=\"802.11b\" packet_size=\"1024\"/>" >> ${OUTFILE};
    echo "</qomet_scenario>" >> ${OUTFILE};

    ./qomet ${OUTFILE};
done
